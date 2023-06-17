#ifndef BLE_DEFINED_H_
#define BLE_DEFINED_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

int BLE_init(void);
void BLE_start(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_DEFINED_H_ */

