#ifndef PICORUBY_NIMBLE_OWNER_H_
#define PICORUBY_NIMBLE_OWNER_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*picoruby_nimble_setup_fn)(void);

#define PICORUBY_NIMBLE_EVT_MAX 104

int picoruby_nimble_start(picoruby_nimble_setup_fn setup);
int picoruby_nimble_stop(void);
bool picoruby_nimble_started(void);

/*
 * Shared host
 *
 * The NimBLE host is one per chip, and services can only be added before it
 * starts. A client describes what it needs from the host; the owner starts
 * the host while at least one client holds it, and runs the setup of every
 * holding client (in the order they first acquired) on each start.
 *
 * picoruby_nimble_start() / stop() above are the BLE class's own client
 * (exclusive: its GATT database is rebuilt on every BLE.new, which needs a
 * host restart, so it cannot share the host with a client holding live
 * connections).
 */
typedef struct picoruby_nimble_client {
  const char *name;
  /* Before the host starts: register GATT services, adjust ble_hs_cfg
   * (security manager settings). Non-zero aborts the start. */
  int (*setup)(void);
  /* Adjusts ble_hs_cfg only (security manager, store): runs after the
   * owner's defaults on every start, and at once when acquiring a running
   * host -- unlike setup, it never makes the host restart. */
  void (*configure)(void);
  /* Host and controller in sync (on every start and after a reset) */
  void (*on_sync)(void);
  /* The host lost its connections: a controller reset (reason from NimBLE)
   * or the owner stopping / restarting the host
   * (PICORUBY_NIMBLE_REASON_STOPPED). Any of the client's connection state
   * is gone. */
  void (*on_reset)(int reason);
  bool exclusive;
  /* owner's */
  bool held;
  bool set_up;
  struct picoruby_nimble_client *next;
} picoruby_nimble_client_t;

#define PICORUBY_NIMBLE_REASON_STOPPED (-1)

/* Hold the host for `client`, starting it when it is down. A client with a
 * setup that has not run yet restarts a running host (its connections drop;
 * the other clients see on_reset). Returns 0, -1 when the host does not
 * start, -2 when an exclusive client is in the way. */
int picoruby_nimble_acquire(picoruby_nimble_client_t *client);
/* Let go; the last holder stops the host. */
void picoruby_nimble_release(picoruby_nimble_client_t *client);
/* Whether a client other than `self` holds the host (`self` NULL: other than
 * the BLE class) */
bool picoruby_nimble_shared(const picoruby_nimble_client_t *self);
/* Whether the host is synced with the controller */
bool picoruby_nimble_synced(void);
uint8_t picoruby_nimble_own_addr_type(void);
void picoruby_nimble_enqueue_event(const uint8_t *pkt, uint16_t len, bool coalesce_adv);
void picoruby_nimble_reset_events(void);
int picoruby_nimble_enqueue_write(uint16_t ruby_handle, const uint8_t *data, uint16_t len);
void picoruby_nimble_reset_writes(void);
void picoruby_nimble_heartbeat_enable(bool enable);
void picoruby_nimble_pump(void);
void picoruby_nimble_attach_vm(void *vm);

#ifdef __cplusplus
}
#endif

#endif /* PICORUBY_NIMBLE_OWNER_H_ */
