/*
 * PicoRuby USB-MIDI Host Driver
 *
 * USB MIDI transport layer for PicoRuby
 */

#ifndef USB_MIDI_HOST_DEFINED_H_
#define USB_MIDI_HOST_DEFINED_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Device status enumeration
 */
typedef enum {
    USB_MIDI_HOST_DISCONNECTED = 0,
    USB_MIDI_HOST_CONNECTED,
    USB_MIDI_HOST_INITIALIZING,
    USB_MIDI_HOST_ERROR
} usb_midi_host_status_t;

/*
 * Device information structure
 */
typedef struct {
    uint16_t vendor_id;
    uint16_t product_id;
    char manufacturer[64];
    char product[64];
    uint8_t midi_in_ep;
    uint8_t midi_out_ep;
} usb_midi_host_device_info_t;

/*
 * Ring buffer for MIDI IN (SPSC - Single Producer Single Consumer)
 */
#define USB_MIDI_HOST_RX_BUFFER_SIZE 1024

typedef struct {
    volatile uint32_t head;  /* Written by USB task */
    volatile uint32_t tail;  /* Written by Ruby task */
    uint8_t data[USB_MIDI_HOST_RX_BUFFER_SIZE];
} usb_midi_host_rx_buffer_t;

/*
 * TX event for FreeRTOS queue
 */
typedef struct {
    uint8_t packet[4];  /* USB-MIDI packet: [CIN+Cable][MIDI1][MIDI2][MIDI3] */
} usb_midi_host_tx_event_t;

/*
 * USB-MIDI Code Index Numbers (CIN)
 */
#define USB_MIDI_HOST_CIN_MISC              0x00
#define USB_MIDI_HOST_CIN_CABLE_EVENT       0x01
#define USB_MIDI_HOST_CIN_SYSCOMMON_2       0x02
#define USB_MIDI_HOST_CIN_SYSCOMMON_3       0x03
#define USB_MIDI_HOST_CIN_SYSEX_START       0x04
#define USB_MIDI_HOST_CIN_SYSCOMMON_1       0x05
#define USB_MIDI_HOST_CIN_SYSEX_END_2       0x06
#define USB_MIDI_HOST_CIN_SYSEX_END_3       0x07
#define USB_MIDI_HOST_CIN_NOTE_OFF          0x08
#define USB_MIDI_HOST_CIN_NOTE_ON           0x09
#define USB_MIDI_HOST_CIN_POLY_KEY          0x0A
#define USB_MIDI_HOST_CIN_CONTROL_CHANGE    0x0B
#define USB_MIDI_HOST_CIN_PROGRAM_CHANGE    0x0C
#define USB_MIDI_HOST_CIN_CHANNEL_PRESSURE  0x0D
#define USB_MIDI_HOST_CIN_PITCH_BEND        0x0E
#define USB_MIDI_HOST_CIN_SINGLE_BYTE       0x0F

/*
 * MIDI Status bytes
 */
#define MIDI_STATUS_NOTE_OFF           0x80
#define MIDI_STATUS_NOTE_ON            0x90
#define MIDI_STATUS_POLY_AFTERTOUCH    0xA0
#define MIDI_STATUS_CONTROL_CHANGE     0xB0
#define MIDI_STATUS_PROGRAM_CHANGE     0xC0
#define MIDI_STATUS_CHANNEL_PRESSURE   0xD0
#define MIDI_STATUS_PITCH_BEND         0xE0
#define MIDI_STATUS_SYSEX_START        0xF0
#define MIDI_STATUS_MTC_QUARTER        0xF1
#define MIDI_STATUS_SONG_POSITION      0xF2
#define MIDI_STATUS_SONG_SELECT        0xF3
#define MIDI_STATUS_TUNE_REQUEST       0xF6
#define MIDI_STATUS_SYSEX_END          0xF7
#define MIDI_STATUS_TIMING_CLOCK       0xF8
#define MIDI_STATUS_START              0xFA
#define MIDI_STATUS_CONTINUE           0xFB
#define MIDI_STATUS_STOP               0xFC
#define MIDI_STATUS_ACTIVE_SENSING     0xFE
#define MIDI_STATUS_SYSTEM_RESET       0xFF

/*
 * Core API
 */

/* Initialize USB MIDI subsystem */
int USB_MIDI_HOST_init(void);

/* Deinitialize USB MIDI subsystem */
void USB_MIDI_HOST_deinit(void);

/* Get current connection status */
usb_midi_host_status_t USB_MIDI_HOST_get_status(void);

/* Get connected device info (returns false if not connected) */
bool USB_MIDI_HOST_get_device_info(usb_midi_host_device_info_t *info);

/*
 * MIDI OUT (Ruby -> USB Device)
 */

/* Send a single USB-MIDI packet */
int USB_MIDI_HOST_send_packet(uint8_t cable, uint8_t cin,
                         uint8_t midi1, uint8_t midi2, uint8_t midi3);

/* Send raw data (multiple packets) */
int USB_MIDI_HOST_send_raw(const uint8_t *data, size_t len);

/*
 * MIDI IN (USB Device -> Ruby)
 */

/* Get number of bytes available in RX buffer */
int USB_MIDI_HOST_bytes_available(void);

/* Read packets from RX buffer */
int USB_MIDI_HOST_read_packet(uint8_t *out_buffer, size_t max_len);

/*
 * Bridge functions (called from USB Host task)
 */

/* Push received data to RX buffer */
void USB_MIDI_HOST_push_rx_data(const uint8_t *data, size_t len);

/* Pop TX packet from queue */
bool USB_MIDI_HOST_pop_tx_packet(uint8_t *out_packet);

/* Get number of packets waiting in TX queue */
int USB_MIDI_HOST_tx_queue_depth(void);

/* Notify device connection */
void USB_MIDI_HOST_notify_connected(const usb_midi_host_device_info_t *info);

/* Notify device disconnection */
void USB_MIDI_HOST_notify_disconnected(void);

/*
 * Reference USB Host driver (ports/esp32)
 *
 * USB_MIDI_HOST_start_driver() bootstraps the ESP-IDF USB Host stack,
 * pins a class-driver task to Core 0 (handles enumeration / interface
 * claim / endpoint setup / hot-plug), and pins a USB-lib-events task
 * to Core 0 that drives usb_host_lib_handle_events(). After this call
 * the gem fully owns the USB Host stack: MIDI devices plugged into
 * the host port enumerate automatically and the ring buffer / TX
 * queue API above just works.
 *
 * Returns 0 on success, -1 on error.
 */
int  USB_MIDI_HOST_start_driver(void);
void USB_MIDI_HOST_stop_driver(void);

#ifdef __cplusplus
}
#endif

#endif /* USB_MIDI_HOST_DEFINED_H_ */
