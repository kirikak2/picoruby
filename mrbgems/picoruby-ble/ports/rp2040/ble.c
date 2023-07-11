#include <stdint.h>
#include <stdbool.h>

#include "../../include/ble.h"

#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_cyw43.h"
#include "pico/stdlib.h"

#include "ble_common.h"

enum BLE_role_t {
  BLE_ROLE_NONE,
  BLE_ROLE_CENTRAL,
  BLE_ROLE_PERIPHERAL
};

static enum BLE_role_t role = BLE_ROLE_NONE;

static btstack_packet_callback_registration_t ble_hci_event_callback_registration;

static uint16_t
att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
  UNUSED(connection_handle);
  BLE_read_value_t read_value = { .att_handle = att_handle, .data = NULL, .size = 0 };
  if (BLE_read_data(&read_value) < 0) return 0;
  return att_read_callback_handle_blob(
           (const uint8_t *)read_value.data,
           read_value.size,
           offset,
           buffer,
           buffer_size
         );
}

static int
att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
  UNUSED(transaction_mode);
  UNUSED(offset);

  if (0 == BLE_write_data(att_handle, (const uint8_t *)buffer, buffer_size)) {
    con_handle = connection_handle;
  }
  return 0;
}

static void
packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  if (packet_type != HCI_EVENT_PACKET) return;
  switch (role) {
    case BLE_ROLE_PERIPHERAL:
      switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
        case HCI_EVENT_DISCONNECTION_COMPLETE:
        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
        case ATT_EVENT_CAN_SEND_NOW:
          BLE_push_event(packet, size);
          break;
        default:
          break;
      }
      break;
    case BLE_ROLE_CENTRAL:
      switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
        case HCI_EVENT_LE_META:
        case GAP_EVENT_ADVERTISING_REPORT:
        case GATT_EVENT_SERVICE_QUERY_RESULT:
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
        case GATT_EVENT_QUERY_COMPLETE:
        case GATT_EVENT_NOTIFICATION:
          BLE_push_event(packet, size);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

int
BLE_init(const uint8_t *profile_data)
{
  l2cap_init();
  sm_init();

  /*
   * Fixme: This looks like a hardcodeing.
   */
  if (profile_data) {
    role = BLE_ROLE_PERIPHERAL;
  } else {
    role = BLE_ROLE_CENTRAL;
  }

  switch (role) {
    case BLE_ROLE_PERIPHERAL:
      att_server_init(profile_data, att_read_callback, att_write_callback);
      att_server_register_packet_handler(packet_handler);
      break;
    case BLE_ROLE_CENTRAL:
      sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
      att_server_init(NULL, NULL, NULL);
      gatt_client_init();
      break;
    default:
      break;
  }

  // inform about BTstack state
  ble_hci_event_callback_registration.callback = &packet_handler;
  hci_add_event_handler(&ble_hci_event_callback_registration);

  return 0;
}

void
BLE_hci_power_control(uint8_t power_mode)
{
  hci_power_control(power_mode);
}

void
BLE_gap_local_bd_addr(uint8_t *local_addr)
{
  gap_local_bd_addr(local_addr);
}

uint8_t
BLE_discover_primary_services(uint16_t conn_handle)
{
  return gatt_client_discover_primary_services(&packet_handler, conn_handle);
}

uint8_t
BLE_discover_characteristics_for_service(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle)
{
  gatt_client_service_t service = {
    .start_group_handle = start_handle,
    .end_group_handle = end_handle,
    .uuid16 = 0,
    .uuid128 = { 0 }
  };
  return gatt_client_discover_characteristics_for_service(&packet_handler, conn_handle, &service);
}

