#include <stdint.h>
#include <stdbool.h>

#include "../../include/ble.h"
#include "../../include/ble_peripheral.h"

#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "pico/stdlib.h"

static hci_con_handle_t con_handle;

void
BLE_peripheral_advertise(uint8_t *adv_data, uint8_t adv_data_len)
{
  // setup advertisements
  uint16_t adv_int_min = 800;
  uint16_t adv_int_max = 800;
  uint8_t adv_type = 0;
  bd_addr_t null_addr;
  memset(null_addr, 0, 6);
  gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
  gap_advertisements_set_data(adv_data_len, adv_data);
  gap_advertisements_enable(1);
}

void
BLE_peripheral_notify(uint16_t att_handle)
{
  BLE_read_value read_value = { .att_handle = att_handle, .data = NULL, .size = 0 };
  if (PeripheralReadData(&read_value) < 0) return;
  if (read_value.size == 0) return;
  att_server_notify(con_handle, att_handle, read_value.data, read_value.size);
}

uint16_t
att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
  UNUSED(connection_handle);
  BLE_read_value read_value = { .att_handle = att_handle, .data = NULL, .size = 0 };
  if (PeripheralReadData(&read_value) < 0) return 0;
  return att_read_callback_handle_blob(
           (const uint8_t *)read_value.data,
           read_value.size,
           offset,
           buffer,
           buffer_size
         );
}

int
att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
  UNUSED(transaction_mode);
  UNUSED(offset);

  if (0 == PeripheralWriteData(att_handle, (const uint8_t *)buffer, buffer_size)) {
    con_handle = connection_handle;
  }
  return 0;
}

static btstack_timer_source_t heartbeat;
static btstack_packet_callback_registration_t hci_event_callback_registration;

static void
heartbeat_handler(struct btstack_timer_source *ts)
{
  ble_heartbeat_on = true;
  // Restart timer
  btstack_run_loop_set_timer(ts, heartbeat_period_ms);
  btstack_run_loop_add_timer(ts);
}

void
BLE_peripheral_request_can_send_now_event(void)
{
  att_server_request_can_send_now_event(con_handle);
}

int
BLE_peripheral_init(const uint8_t *profile_data)
{
  l2cap_init();
  sm_init();

  att_server_init(profile_data, att_read_callback, att_write_callback);

  // inform about BTstack state
  hci_event_callback_registration.callback = &packet_handler;
  hci_add_event_handler(&hci_event_callback_registration);

  // register for ATT event
  att_server_register_packet_handler(packet_handler);

  // set one-shot btstack timer
  heartbeat.process = &heartbeat_handler;
  btstack_run_loop_set_timer(&heartbeat, heartbeat_period_ms);
  btstack_run_loop_add_timer(&heartbeat);
  return 0;
}

