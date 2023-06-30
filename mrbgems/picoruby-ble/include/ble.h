#ifndef BLE_DEFINED_H_
#define BLE_DEFINED_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool ble_heartbeat_on;

uint16_t PeripheralReadTemperature(uint8_t **data);

int BLE_init(const uint8_t *profile);
void BLE_hci_power_on(void);

uint8_t BLE_packet_event(void);
void BLE_down_packet_flag(void);
void BLE_advertise(uint8_t *adv_data, uint8_t adv_data_len);
void BLE_enable_le_notification(void);
void BLE_notify(void);
void BLE_gap_local_bd_addr(uint8_t *local_addr);

bool BLE_le_notification_enabled_q(void);
void BLE_request_can_send_now_event(void);
void BLE_cyw43_arch_gpio_put(uint8_t pin, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* BLE_DEFINED_H_ */

