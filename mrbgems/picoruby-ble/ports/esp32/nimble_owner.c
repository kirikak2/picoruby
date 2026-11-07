#include "nimble_owner.h"
#include "ble_common.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "prb_ble_evq";

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"

#include "../../include/ble.h"

#define EVQ_DEPTH 32
#define EVQ_PKT_MAX PICORUBY_NIMBLE_EVT_MAX
#define WRQ_DEPTH 32
#define WRQ_PKT_MAX 256
#define WRQ_RUBY_PENDING_MAX 16
#define HEARTBEAT_PERIOD_US (1000 * 1000)
#define SYNC_TIMEOUT_TICKS pdMS_TO_TICKS(2000)

typedef struct {
  uint16_t len;
  bool is_adv;
  uint8_t data[EVQ_PKT_MAX];
} evq_entry_t;

static evq_entry_t evq[EVQ_DEPTH];
static int evq_head = 0;
static int evq_count = 0;
static uint32_t evq_gen = 0;
static portMUX_TYPE evq_mux = portMUX_INITIALIZER_UNLOCKED;

typedef struct {
  uint16_t ruby_handle;
  uint16_t len;
  uint8_t data[WRQ_PKT_MAX];
} wrq_entry_t;

static wrq_entry_t wrq[WRQ_DEPTH];
static int wrq_head = 0;
static int wrq_count = 0;
static uint32_t wrq_dropped = 0;
static bool wrq_drop_logged = false;
static portMUX_TYPE wrq_mux = portMUX_INITIALIZER_UNLOCKED;

static bool started = false;

/* Clients in the order they first acquired; the BLE class's is legacy_client */
static picoruby_nimble_client_t *clients = NULL;
static picoruby_nimble_client_t legacy_client = {
  .name = "BLE",
  .priority = 1,                      /* the running script's */
  .on_sync = picoruby_ble_on_sync,
  .on_reset = picoruby_ble_on_reset,
};
/* The BLE class's GATT services are in the host's database */
static bool legacy_gatt = false;

/* Holders of the procedures run one at a time (picoruby_nimble_claim) */
static picoruby_nimble_client_t *proc_holder[PICORUBY_NIMBLE_PROC_COUNT];
static portMUX_TYPE proc_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool synced = false;
static uint8_t own_addr_type = BLE_OWN_ADDR_PUBLIC;
static SemaphoreHandle_t sync_sem = NULL;
static esp_timer_handle_t heartbeat_timer = NULL;
static int heartbeat_depth = 0; // nest count: only the outermost disable actually stops it
static volatile bool heartbeat_pending = false;

void
picoruby_nimble_enqueue_event(const uint8_t *pkt, uint16_t len, bool coalesce_adv)
{
  if (len > EVQ_PKT_MAX) {
    len = EVQ_PKT_MAX;
  }
  taskENTER_CRITICAL(&evq_mux);
  int slot = -1;
  if (coalesce_adv && evq_count > 0) {
    int tail = (evq_head + evq_count - 1) % EVQ_DEPTH;
    if (evq[tail].is_adv) slot = tail;
  }
  if (slot == evq_head) evq_gen++;
  if (slot < 0) {
    if (evq_count == EVQ_DEPTH) {
      if (coalesce_adv) {
        taskEXIT_CRITICAL(&evq_mux);
        return;
      }
      evq_head = (evq_head + 1) % EVQ_DEPTH;
      evq_count--;
      evq_gen++;
    }
    slot = (evq_head + evq_count) % EVQ_DEPTH;
    evq_count++;
  }
  evq[slot].len = len;
  evq[slot].is_adv = coalesce_adv;
  memcpy(evq[slot].data, pkt, len);
  taskEXIT_CRITICAL(&evq_mux);
}

void
picoruby_nimble_reset_events(void)
{
  taskENTER_CRITICAL(&evq_mux);
  evq_head = 0;
  evq_count = 0;
  evq_gen++;
  taskEXIT_CRITICAL(&evq_mux);
  heartbeat_pending = false;
}

int
picoruby_nimble_enqueue_write(uint16_t ruby_handle, const uint8_t *data, uint16_t len)
{
  if (ruby_handle == 0 || len == 0 || len > WRQ_PKT_MAX) return -1;
  taskENTER_CRITICAL(&wrq_mux);
  if (wrq_count == WRQ_DEPTH) {
    wrq_dropped++;
    bool first = !wrq_drop_logged;
    wrq_drop_logged = true;
    taskEXIT_CRITICAL(&wrq_mux);
    if (first) {
      ESP_LOGW(TAG, "write queue full (depth=%d), dropping; total dropped=%lu",
               WRQ_DEPTH, (unsigned long)wrq_dropped);
    }
    return -1;
  }
  int slot = (wrq_head + wrq_count) % WRQ_DEPTH;
  wrq_count++;
  wrq[slot].ruby_handle = ruby_handle;
  wrq[slot].len = len;
  memcpy(wrq[slot].data, data, len);
  taskEXIT_CRITICAL(&wrq_mux);
  return 0;
}

void
picoruby_nimble_reset_writes(void)
{
  taskENTER_CRITICAL(&wrq_mux);
  wrq_head = 0;
  wrq_count = 0;
  wrq_drop_logged = false;
  taskEXIT_CRITICAL(&wrq_mux);
}

static void
flush_writes(void)
{
  static uint8_t buf[WRQ_PKT_MAX];
  for (;;) {
    uint16_t handle;
    uint16_t len;
    taskENTER_CRITICAL(&wrq_mux);
    if (wrq_count == 0) {
      taskEXIT_CRITICAL(&wrq_mux);
      return;
    }
    handle = wrq[wrq_head].ruby_handle;
    len = wrq[wrq_head].len;
    memcpy(buf, wrq[wrq_head].data, len);
    taskEXIT_CRITICAL(&wrq_mux);
    if (BLE_write_pending(handle) >= WRQ_RUBY_PENDING_MAX) return;
    if (BLE_write_data(handle, buf, len) != 0) return;
    taskENTER_CRITICAL(&wrq_mux);
    wrq_head = (wrq_head + 1) % WRQ_DEPTH;
    wrq_count--;
    taskEXIT_CRITICAL(&wrq_mux);
  }
}

static void
drain_events(void)
{
  static uint8_t buf[EVQ_PKT_MAX];
  for (;;) {
    uint16_t len;
    uint32_t gen;
    taskENTER_CRITICAL(&evq_mux);
    if (evq_count == 0) {
      taskEXIT_CRITICAL(&evq_mux);
      return;
    }
    len = evq[evq_head].len;
    memcpy(buf, evq[evq_head].data, len);
    gen = evq_gen;
    taskEXIT_CRITICAL(&evq_mux);
    if (BLE_push_event(buf, len) != 0) return;
    taskENTER_CRITICAL(&evq_mux);
    if (gen == evq_gen) {
      evq_head = (evq_head + 1) % EVQ_DEPTH;
      evq_count--;
    }
    taskEXIT_CRITICAL(&evq_mux);
  }
}

static void
heartbeat_timer_cb(void *arg)
{
  (void)arg;
  heartbeat_pending = true;
}

void
picoruby_nimble_pump(void)
{
  flush_writes();
  picoruby_ble_refresh_read_mirrors();
  drain_events();
  if (heartbeat_pending) {
    heartbeat_pending = false;
    if (BLE_heartbeat() != 0) heartbeat_pending = true;
  }
}

void
picoruby_nimble_heartbeat_enable(bool enable)
{
  if (heartbeat_timer == NULL) return;
  if (enable) {
    if (heartbeat_depth++ > 0) return;
    esp_timer_start_periodic(heartbeat_timer, HEARTBEAT_PERIOD_US);
  } else {
    if (heartbeat_depth > 0) heartbeat_depth--;
    if (heartbeat_depth > 0) return;
    esp_timer_stop(heartbeat_timer);
  }
}

// Unconditional stop, not depth-gated: runs on every BLE_init and must not carry nesting state across sessions.
static void
heartbeat_force_stop(void)
{
  heartbeat_depth = 0;
  if (heartbeat_timer) esp_timer_stop(heartbeat_timer);
}

static void
on_sync(void)
{
  ble_hs_util_ensure_addr(0);
  if (ble_hs_id_infer_auto(0, &own_addr_type) != 0) {
    own_addr_type = 0;
  }
  synced = true;
  if (sync_sem) xSemaphoreGive(sync_sem);
  for (picoruby_nimble_client_t *c = clients; c; c = c->next) {
    if (c->held && c->set_up && c->on_sync) c->on_sync();
  }
}

static void clear_procs(void);

static void
on_reset(int reason)
{
  synced = false;
  clear_procs();          /* the controller dropped advertising and scans */
  for (picoruby_nimble_client_t *c = clients; c; c = c->next) {
    if (c->held && c->on_reset) c->on_reset(reason);
  }
}

static void
host_task(void *param)
{
  (void)param;
  nimble_port_run();
  nimble_port_freertos_deinit();
}

static void
ensure_timers(void)
{
  if (heartbeat_timer == NULL) {
    const esp_timer_create_args_t hargs = {
      .callback = heartbeat_timer_cb,
      .name = "prb_ble_hb",
    };
    esp_timer_create(&hargs, &heartbeat_timer);
  }
}

static SemaphoreHandle_t owner_lock = NULL;
static portMUX_TYPE owner_lock_mux = portMUX_INITIALIZER_UNLOCKED;

static void
lock_owner(void)
{
  if (owner_lock == NULL) {
    SemaphoreHandle_t m = xSemaphoreCreateRecursiveMutex();
    taskENTER_CRITICAL(&owner_lock_mux);
    if (owner_lock == NULL) {
      owner_lock = m;
      m = NULL;
    }
    taskEXIT_CRITICAL(&owner_lock_mux);
    if (m) vSemaphoreDelete(m);
  }
  xSemaphoreTakeRecursive(owner_lock, portMAX_DELAY);
}

static void
unlock_owner(void)
{
  xSemaphoreGiveRecursive(owner_lock);
}

static void
notify_reset(int reason)
{
  for (picoruby_nimble_client_t *c = clients; c; c = c->next) {
    if (c->held && c->on_reset) c->on_reset(reason);
  }
}

static int
host_start(void)
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    return -1;
  }

  if (nimble_port_init() != ESP_OK) {
    return -1;
  }

  ble_hs_cfg.sync_cb = on_sync;
  ble_hs_cfg.reset_cb = on_reset;
  ble_hs_cfg.gatts_register_cb = picoruby_ble_gatts_register_cb;
  ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
  ble_hs_cfg.sm_bonding = 0;
  ble_hs_cfg.sm_mitm = 0;
  ble_hs_cfg.sm_sc = 0;

  for (picoruby_nimble_client_t *c = clients; c; c = c->next) {
    if (c->held && c->configure) c->configure();
  }
  for (picoruby_nimble_client_t *c = clients; c; c = c->next) {
    c->set_up = false;
    if (!c->held) continue;
    if (c->setup && c->setup() != 0) {
      ESP_LOGE(TAG, "NimBLE client %s: setup failed", c->name ? c->name : "?");
      nimble_port_deinit();
      return -1;
    }
    c->set_up = true;
  }

  ensure_timers();
  if (!legacy_client.held) {
    /* (a restart for another client keeps the BLE class's VM and queues) */
    picoruby_nimble_attach_vm(NULL);
    picoruby_nimble_reset_events();
    picoruby_nimble_reset_writes();
  }

  if (sync_sem == NULL) sync_sem = xSemaphoreCreateBinary();
  xSemaphoreTake(sync_sem, 0);
  synced = false;
  started = true;  /* before the host task: on_sync runs on it */
  nimble_port_freertos_init(host_task);

  if (xSemaphoreTake(sync_sem, SYNC_TIMEOUT_TICKS) != pdTRUE) {
    started = false;
    nimble_port_stop();
    nimble_port_deinit();
    return -1;
  }
  return 0;
}

static void
clear_procs(void)
{
  taskENTER_CRITICAL(&proc_mux);
  for (int i = 0; i < PICORUBY_NIMBLE_PROC_COUNT; i++) proc_holder[i] = NULL;
  for (picoruby_nimble_client_t *c = clients; c; c = c->next) c->waiting = 0;
  taskEXIT_CRITICAL(&proc_mux);
}

static void
host_stop(void)
{
  if (!started) return;
  notify_reset(PICORUBY_NIMBLE_REASON_STOPPED);
  clear_procs();
  if (!legacy_client.held) heartbeat_force_stop();
  ble_gap_adv_stop();
  ble_gap_disc_cancel();
  nimble_port_stop();
  nimble_port_deinit();
  synced = false;
  started = false;
  for (picoruby_nimble_client_t *c = clients; c; c = c->next) c->set_up = false;
}

static bool
held_by_other(const picoruby_nimble_client_t *self, bool exclusive_only)
{
  for (picoruby_nimble_client_t *c = clients; c; c = c->next) {
    if (c == self || !c->held) continue;
    if (!exclusive_only || c->exclusive) return true;
  }
  return false;
}

int
picoruby_nimble_acquire(picoruby_nimble_client_t *client)
{
  if (client == NULL) return -1;
  lock_owner();
  if (held_by_other(client, true) || (client->exclusive && held_by_other(client, false))) {
    unlock_owner();
    return -2;
  }
  bool known = false;
  picoruby_nimble_client_t **tail = &clients;
  for (; *tail; tail = &(*tail)->next) {
    if (*tail == client) known = true;
  }
  if (!known) {
    client->next = NULL;
    *tail = client;
  }
  bool was_held = client->held;
  client->held = true;
  int rc = 0;
  if (!started) {
    rc = host_start();
  } else if (client->setup && !client->set_up) {
    /* Services can only be added to a stopped host */
    host_stop();
    rc = host_start();
  } else if (!was_held) {
    if (client->configure) client->configure();
    client->set_up = true;
    if (synced && client->on_sync) client->on_sync();
  }
  if (rc != 0) {
    client->held = false;
    /* the others had the host before: bring it back for them */
    if (!started && held_by_other(client, false)) host_start();
  }
  unlock_owner();
  return rc;
}

void
picoruby_nimble_release(picoruby_nimble_client_t *client)
{
  if (client == NULL) return;
  lock_owner();
  if (client->held) {
    for (int i = 0; i < PICORUBY_NIMBLE_PROC_COUNT; i++) picoruby_nimble_unclaim(client, i);
    client->held = false;
    client->waiting = 0;
    if (!held_by_other(client, false)) host_stop();
  }
  unlock_owner();
}

/* --- procedures run one at a time ---------------------------------- */

static picoruby_nimble_client_t *
client_or_legacy(picoruby_nimble_client_t *c)
{
  return c ? c : &legacy_client;
}

bool
picoruby_nimble_claim(picoruby_nimble_client_t *client, int proc)
{
  if (proc < 0 || proc >= PICORUBY_NIMBLE_PROC_COUNT) return false;
  client = client_or_legacy(client);
  picoruby_nimble_client_t *victim = NULL;
  bool ok;
  taskENTER_CRITICAL(&proc_mux);
  picoruby_nimble_client_t *h = proc_holder[proc];
  if (h == NULL || h == client) {
    ok = true;
  } else if (client->priority > h->priority) {
    ok = true;
    victim = h;
    h->waiting |= (uint8_t)(1u << proc);
  } else {
    ok = false;
    client->waiting |= (uint8_t)(1u << proc);
  }
  if (ok) {
    proc_holder[proc] = client;
    client->waiting &= (uint8_t)~(1u << proc);
  }
  taskEXIT_CRITICAL(&proc_mux);
  if (victim) {
    ESP_LOGI(TAG, "%s takes %s from %s", client->name ? client->name : "?",
             proc == PICORUBY_NIMBLE_PROC_ADV ? "advertising" : "scanning",
             victim->name ? victim->name : "?");
    if (victim->yield) victim->yield(proc);
  }
  return ok;
}

void
picoruby_nimble_unclaim(picoruby_nimble_client_t *client, int proc)
{
  if (proc < 0 || proc >= PICORUBY_NIMBLE_PROC_COUNT) return;
  client = client_or_legacy(client);
  enum { MAX_WAKE = 8 };
  picoruby_nimble_client_t *wake[MAX_WAKE];
  int n = 0;
  taskENTER_CRITICAL(&proc_mux);
  if (proc_holder[proc] != client) {
    taskEXIT_CRITICAL(&proc_mux);
    return;
  }
  proc_holder[proc] = NULL;
  for (picoruby_nimble_client_t *c = clients; c && n < MAX_WAKE; c = c->next) {
    if (c != client && c->held && (c->waiting & (1u << proc))) {
      c->waiting &= (uint8_t)~(1u << proc);
      wake[n++] = c;
    }
  }
  taskEXIT_CRITICAL(&proc_mux);
  for (int i = 0; i < n; i++) {
    if (wake[i]->proc_free) wake[i]->proc_free(proc);
  }
}

bool
picoruby_nimble_holds(const picoruby_nimble_client_t *client, int proc)
{
  if (proc < 0 || proc >= PICORUBY_NIMBLE_PROC_COUNT) return false;
  const picoruby_nimble_client_t *c = client ? client : &legacy_client;
  taskENTER_CRITICAL(&proc_mux);
  bool held = (proc_holder[proc] == c);
  taskEXIT_CRITICAL(&proc_mux);
  return held;
}

bool
picoruby_nimble_gap_provided(void)
{
  for (picoruby_nimble_client_t *c = clients; c; c = c->next) {
    if (c->held && c->provides_gap) return true;
  }
  return false;
}

bool
picoruby_nimble_shared(const picoruby_nimble_client_t *self)
{
  bool shared;
  lock_owner();
  shared = held_by_other(self ? self : &legacy_client, false);
  unlock_owner();
  return shared;
}

bool
picoruby_nimble_synced(void)
{
  return started && synced;
}

/* The BLE class (BLE.new): `setup` registers its GATT services, NULL when
 * it has none. Its queues start empty; the VM attaches after. */
int
picoruby_nimble_start(picoruby_nimble_setup_fn setup)
{
  lock_owner();
  picoruby_ble_teardown();          /* the previous BLE.new's procedures and link */
  heartbeat_force_stop();
  picoruby_nimble_attach_vm(NULL);
  picoruby_nimble_reset_events();
  picoruby_nimble_reset_writes();

  bool restart = legacy_client.held && started && (setup != NULL || legacy_gatt);
  legacy_client.setup = setup;
  int rc;
  if (restart) {
    /* services can only change on a stopped host */
    host_stop();
    rc = host_start();
    if (rc != 0) {
      legacy_client.held = false;
      if (held_by_other(&legacy_client, false)) host_start();   /* back for the others */
    }
  } else if (legacy_client.held && started) {
    rc = 0;
  } else {
    if (legacy_client.held) legacy_client.held = false;   /* host went down under it */
    rc = picoruby_nimble_acquire(&legacy_client);
  }
  legacy_gatt = (rc == 0 && setup != NULL);
  unlock_owner();
  return rc == 0 ? 0 : -1;
}

/* The BLE class lets go (script end; its VM may be gone already) */
int
picoruby_nimble_stop(void)
{
  lock_owner();
  if (legacy_client.held) {
    picoruby_ble_teardown();
    heartbeat_force_stop();
    picoruby_nimble_attach_vm(NULL);
    picoruby_nimble_reset_events();
    picoruby_nimble_reset_writes();
    bool drop_services = legacy_gatt && held_by_other(&legacy_client, false);
    legacy_client.setup = NULL;
    legacy_gatt = false;
    picoruby_nimble_release(&legacy_client);
    if (drop_services && started) {
      /* take its services out of the database the others go on with */
      host_stop();
      host_start();
    }
  }
  unlock_owner();
  return 0;
}

bool
picoruby_nimble_started(void)
{
  return started;
}

uint8_t
picoruby_nimble_own_addr_type(void)
{
  return own_addr_type;
}
