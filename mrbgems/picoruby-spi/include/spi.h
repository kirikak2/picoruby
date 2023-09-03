#ifndef SPI_DEFINED_H_
#define SPI_DEFINED_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PICORUBY_SPI_BITBANG          0
#define PICORUBY_SPI_RP2040_SPI0      1
#define PICORUBY_SPI_RP2040_SPI1      2

#define UNIT_SELECT() \
  do { \
    switch (unit_num) { \
      case PICORUBY_SPI_BITBANG:     { unit = NULL; break; } \
      case PICORUBY_SPI_RP2040_SPI0: { unit = spi0; break; } \
      case PICORUBY_SPI_RP2040_SPI1: { unit = spi1; break; } \
      default: { return ERROR_INVALID_UNIT; } \
    } \
  } while (0)

typedef enum {
  ERROR_NONE              =  0,
  ERROR_INVALID_UNIT      = -1,
  ERROR_INVALID_MODE      = -2,
  ERROR_INVALID_FIRST_BIT = -3,
  ERROR_NOT_IMPLEMENTED   = -4,
} spi_status_t;


int SPI_unit_name_to_unit_num(const char *);
spi_status_t SPI_gpio_init(int, uint32_t, int8_t, int8_t, int8_t, uint8_t, uint8_t, uint8_t);
int SPI_read_blocking(int, uint8_t*, size_t, uint8_t);
int SPI_write_blocking(int, uint8_t*, size_t);
int SPI_transfer(int, uint8_t*, uint8_t*, size_t);

#ifdef __cplusplus
}
#endif

#endif /* SPI_DEFINED_H_ */

