#ifndef BLE_COMMON_DEFINED_H_
#define BLE_COMMON_DEFINED_H_

#include <stdint.h>
#include <stdbool.h>

#include "host/ble_gap.h"
#include "host/ble_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t con_handle;

#define EVT_ATT_CAN_SEND_NOW 0xB7

int picoruby_ble_gap_event(struct ble_gap_event *event, void *arg);
void picoruby_ble_gatts_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
uint16_t picoruby_ble_handle_r2n(uint16_t ruby_handle);
void picoruby_ble_refresh_read_mirrors(void);
void picoruby_ble_peripheral_rearm_adv(void);

/* The BLE class's side of the shared host (nimble_owner.c) */
void picoruby_ble_on_sync(void);
void picoruby_ble_on_reset(int reason);
/* Stop the BLE class's advertising / scan / connection setup, drop its link */
void picoruby_ble_teardown(void);
/* Advertising / scan ended on their own (connected, completed) */
void picoruby_ble_peripheral_adv_ended(void);
void picoruby_ble_central_proc_ended(void);
bool picoruby_ble_central_rescan(void);
void picoruby_ble_central_teardown(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_COMMON_DEFINED_H_ */
