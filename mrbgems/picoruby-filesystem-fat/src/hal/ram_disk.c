#include <stdlib.h>
#include <string.h>

/* mruby/c */
#include <alloc.h>

#include "ram_disk.h"

/* Disk Status Bits (DSTATUS) */
#define STA_NOINIT      0x01    /* Drive not initialized */
#define STA_NODISK      0x02    /* No medium in the drive */
#define STA_PROTECT     0x04    /* Write protected */

#define DISK_ERASE_UNIT_SIZE    512
/*
 * Volume size = SECTOR_SIZE * SECTOR_COUNT
 *  512 * 192 == 96 KiB
 *  Seems this is the minimum volume size of FatFS
 */
#define SECTOR_SIZE             512
#define SECTOR_COUNT            192

static BYTE *ram_disk = NULL;

int
RAM_disk_initialize(void)
{
  if (ram_disk) return 0;
  ram_disk = mrbc_raw_alloc(SECTOR_SIZE * SECTOR_COUNT);
  if (ram_disk) {
    return 0;
  } else {
    return STA_NODISK;
  }
}

int
RAM_disk_status(void)
{
  if (ram_disk) {
    return 0;
  } else {
    return STA_NOINIT;
  }
}

int
RAM_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
  memcpy(buff, ram_disk + sector * SECTOR_SIZE, count * SECTOR_SIZE);
  return 0;
}

int
RAM_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
  memcpy(ram_disk + sector * SECTOR_SIZE, buff, count * SECTOR_SIZE);
  return 0;
}

DWORD
get_fattime(void)
{
  DWORD time = 1<<21 | 1<<16;
  return time;
}

DRESULT
RAM_disk_ioctl(BYTE cmd, void *buff)
{
  switch (cmd) {
    case CTRL_SYNC:
      // TODO
      break;
    case GET_BLOCK_SIZE:
      *((DWORD *)buff) = 1; // Non flash memory returns 1
      break;
    case CTRL_TRIM:
      // TODO *((LBA_t *)buff) = (LBA_t)something
      return RES_ERROR;
    case GET_SECTOR_SIZE:
      *((WORD *)buff) = (WORD)SECTOR_SIZE;
      break;
    case GET_SECTOR_COUNT:
      *((LBA_t *)buff) = (LBA_t)SECTOR_COUNT;
      break;
    default :
      return RES_PARERR;
  }
  return RES_OK;
}
