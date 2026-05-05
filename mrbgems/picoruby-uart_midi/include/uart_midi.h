/*
 * PicoRuby UART/Serial MIDI Driver
 *
 * Generic UART-based MIDI transport layer for PicoRuby. Speaks
 * standard 5-pin MIDI DIN (31250 baud, 8N1) by default but accepts
 * any other baud rate at init time, so the same gem covers MIDI-DIN
 * adapters, on-board synth chips (e.g. SAM2695) and any future
 * UART-attached MIDI peripheral.
 *
 * Single-instance for now: one UART_MIDI port per build. Multi-port
 * support is a future enhancement.
 */

#ifndef UART_MIDI_DEFINED_H_
#define UART_MIDI_DEFINED_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Device status enumeration.
 */
typedef enum {
    UART_MIDI_NOT_INITIALIZED = 0,
    UART_MIDI_READY,
    UART_MIDI_ERROR
} uart_midi_status_t;

/*
 * Device information structure.
 */
typedef struct {
    uint32_t baud_rate;
    int      uart_num;
    int      tx_pin;
    int      rx_pin;  /* -1 if RX disabled (TX-only mode) */
} uart_midi_device_info_t;

/*
 * USB-MIDI Code Index Numbers (CIN). Same numeric values as USB-MIDI
 * 1.0 so packets can be passed through unchanged from picoruby-midi's
 * scheduler / clock paths.
 */
#define UART_MIDI_CIN_MISC              0x00
#define UART_MIDI_CIN_CABLE_EVENT       0x01
#define UART_MIDI_CIN_SYSCOMMON_2       0x02
#define UART_MIDI_CIN_SYSCOMMON_3       0x03
#define UART_MIDI_CIN_SYSEX_START       0x04
#define UART_MIDI_CIN_SYSCOMMON_1       0x05
#define UART_MIDI_CIN_SYSEX_END_2       0x06
#define UART_MIDI_CIN_SYSEX_END_3       0x07
#define UART_MIDI_CIN_NOTE_OFF          0x08
#define UART_MIDI_CIN_NOTE_ON           0x09
#define UART_MIDI_CIN_POLY_KEY          0x0A
#define UART_MIDI_CIN_CONTROL_CHANGE    0x0B
#define UART_MIDI_CIN_PROGRAM_CHANGE    0x0C
#define UART_MIDI_CIN_CHANNEL_PRESSURE  0x0D
#define UART_MIDI_CIN_PITCH_BEND        0x0E
#define UART_MIDI_CIN_SINGLE_BYTE       0x0F

/*
 * MIDI Status bytes (subset; full list lives in the protocol header).
 */
#define UART_MIDI_STATUS_NOTE_OFF        0x80
#define UART_MIDI_STATUS_NOTE_ON         0x90
#define UART_MIDI_STATUS_POLY_AFTERTOUCH 0xA0
#define UART_MIDI_STATUS_CONTROL_CHANGE  0xB0
#define UART_MIDI_STATUS_PROGRAM_CHANGE  0xC0
#define UART_MIDI_STATUS_CHANNEL_PRESSURE 0xD0
#define UART_MIDI_STATUS_PITCH_BEND      0xE0
#define UART_MIDI_STATUS_SYSEX_START     0xF0
#define UART_MIDI_STATUS_SYSEX_END       0xF7
#define UART_MIDI_STATUS_TIMING_CLOCK    0xF8
#define UART_MIDI_STATUS_START           0xFA
#define UART_MIDI_STATUS_CONTINUE        0xFB
#define UART_MIDI_STATUS_STOP            0xFC
#define UART_MIDI_STATUS_ACTIVE_SENSING  0xFE
#define UART_MIDI_STATUS_SYSTEM_RESET    0xFF

/* Standard MIDI DIN baud rate. */
#define UART_MIDI_DEFAULT_BAUD_RATE 31250

/*
 * Core API
 */

/* Initialize UART for MIDI I/O.
 * @param tx_pin GPIO pin for MIDI TX (output to device); required
 * @param rx_pin GPIO pin for MIDI RX (input from device); -1 to disable
 * @param baud   UART baud rate; 0 to use UART_MIDI_DEFAULT_BAUD_RATE
 * @return 0 on success, -1 on error.
 */
int UART_MIDI_init(int tx_pin, int rx_pin, uint32_t baud);

/* Tear down the UART driver. */
void UART_MIDI_deinit(void);

/* Current driver status. */
uart_midi_status_t UART_MIDI_get_status(void);

/* Read back the configured device info (baud / pins / uart_num).
 * Returns false if not initialized. */
bool UART_MIDI_get_device_info(uart_midi_device_info_t *info);

/*
 * MIDI OUT (host -> device)
 */

/* Send one USB-MIDI 4-byte packet. The cable arg is ignored (UART is
 * a single channel). cin is consulted to know how many bytes to push
 * out the wire. */
int UART_MIDI_send_packet(uint8_t cable, uint8_t cin,
                          uint8_t midi1, uint8_t midi2, uint8_t midi3);

/* Send raw MIDI bytes straight through. */
int UART_MIDI_send_raw(const uint8_t *data, size_t len);

/*
 * MIDI IN (device -> host)
 */

int  UART_MIDI_Input_init(void);
void UART_MIDI_Input_deinit(void);
int  UART_MIDI_Input_start(void);
void UART_MIDI_Input_stop(void);
bool UART_MIDI_Input_is_running(void);
bool UART_MIDI_Input_was_started(void);

int  UART_MIDI_bytes_available(void);
int  UART_MIDI_read_bytes(uint8_t *out_buffer, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* UART_MIDI_DEFINED_H_ */
