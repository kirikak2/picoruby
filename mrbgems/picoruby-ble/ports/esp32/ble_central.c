#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../../include/ble.h"
#include "../../include/ble_central.h"

#include "host/ble_hs.h"
#include "host/ble_gap.h"

#include "ble_common.h"
#include "nimble_owner.h"

static struct ble_gap_disc_params scan_params = {
  .itvl = 0x60,
  .window = 0x30,
  .passive = 1,
};

/**
 * @param scan_type 0 = passive, 1 = active
 * @param scan_interval range 0x0004..0x4000, unit 0.625 ms
 * @param scan_window range 0x0004..0x4000, unit 0.625 ms
 * @param scanning_filter_policy 0 = all devices, 1 = all from whitelist
 */
void
BLE_central_set_scan_params(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window, uint8_t scanning_filter_policy)
{
  scan_params.itvl = scan_interval;
  scan_params.window = scan_window;
  scan_params.passive = (scan_type == 0);
  scan_params.filter_policy = scanning_filter_policy;
  scan_params.filter_duplicates = 0;
}

/* Scanning and connection setup are claimed from the shared host
 * (nimble_owner.h); the BLE class has priority over BLE-MIDI's scans. */
static bool scan_wanted = false;
static bool connecting = false;

static int
scan_start(void)
{
  if (!picoruby_nimble_claim(NULL, PICORUBY_NIMBLE_PROC_CENTRAL)) return BLE_HS_EBUSY;
  if (ble_gap_disc_active()) ble_gap_disc_cancel();   /* ours, with new parameters */
  int rc = ble_gap_disc(picoruby_nimble_own_addr_type(), BLE_HS_FOREVER,
                        &scan_params, picoruby_ble_gap_event, NULL);
  if (rc != 0 && !connecting) picoruby_nimble_unclaim(NULL, PICORUBY_NIMBLE_PROC_CENTRAL);
  return rc;
}

void
BLE_central_start_scan(void)
{
  scan_wanted = true;
  scan_start();
}

void
BLE_central_stop_scan(void)
{
  scan_wanted = false;
  if (picoruby_nimble_holds(NULL, PICORUBY_NIMBLE_PROC_CENTRAL)) {
    if (ble_gap_disc_active()) ble_gap_disc_cancel();
    if (!connecting) picoruby_nimble_unclaim(NULL, PICORUBY_NIMBLE_PROC_CENTRAL);
  }
}

/* The scan completed or the connection setup ended (connected or not) */
void
picoruby_ble_central_proc_ended(void)
{
  connecting = false;
  if (!(scan_wanted && ble_gap_disc_active())) {
    scan_wanted = false;
    picoruby_nimble_unclaim(NULL, PICORUBY_NIMBLE_PROC_CENTRAL);
  }
}

/* After a host restart: scan again if a scan was going */
bool
picoruby_ble_central_rescan(void)
{
  connecting = false;
  if (!scan_wanted) return false;
  return scan_start() == 0;
}

void
picoruby_ble_central_teardown(void)
{
  scan_wanted = false;
  if (picoruby_nimble_holds(NULL, PICORUBY_NIMBLE_PROC_CENTRAL)) {
    if (ble_gap_disc_active()) ble_gap_disc_cancel();
    if (connecting) ble_gap_conn_cancel();
  }
  connecting = false;
  picoruby_nimble_unclaim(NULL, PICORUBY_NIMBLE_PROC_CENTRAL);
}

uint8_t
BLE_central_gap_connect(const uint8_t *addr, uint8_t addr_type)
{
  if (!picoruby_nimble_claim(NULL, PICORUBY_NIMBLE_PROC_CENTRAL)) return 0xff;
  if (ble_gap_disc_active()) {
    ble_gap_disc_cancel();          /* ours: the others gave way */
  }
  scan_wanted = false;

  ble_addr_t peer;
  peer.type = addr_type;
  for (int i = 0; i < 6; i++) peer.val[i] = addr[5 - i];

  struct ble_gap_conn_params params = {
    .scan_itvl = 60000 / 625,
    .scan_window = 30000 / 625,
    .itvl_min = 10000 / 1250,
    .itvl_max = 30000 / 1250,
    .latency = 4,
    .supervision_timeout = 7200 / 10,
    .min_ce_len = 10000 / 625,
    .max_ce_len = 30000 / 625,
  };

  connecting = true;
  int rc = ble_gap_connect(picoruby_nimble_own_addr_type(), &peer, 30000,
                           &params, picoruby_ble_gap_event, NULL);
  if (rc == 0) return 0;
  connecting = false;
  picoruby_nimble_unclaim(NULL, PICORUBY_NIMBLE_PROC_CENTRAL);
  return (rc & 0xff) ? (uint8_t)rc : 0xff;
}
