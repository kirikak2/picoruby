# picoruby-uart_midi

UART/Serial MIDI transport layer for PicoRuby.

Generic UART-based MIDI transport. Speaks classic 5-pin MIDI DIN at
31250 baud by default; the third arg lets you override the baud for
chips like the SAM2695 that speak MIDI over a non-standard rate. Use
`picoruby-midi` for high-level MIDI operations (note triggering, clock,
input dispatch).

## Usage

```ruby
require 'midi'
require 'uart_midi'

uart = UART_MIDI.new(17, 18)            # tx_pin, rx_pin (MIDI DIN)
device = MIDI::Device.new(uart)
device.note_on(60, 100)
sleep 1
device.note_off(60)
```

TX-only (no RX wiring):

```ruby
uart = UART_MIDI.new(17, -1)            # rx_pin = -1 disables RX
```

Custom baud (rare; for non-standard MIDI peripherals):

```ruby
uart = UART_MIDI.new(13, -1, 38400)     # tx_pin, rx_pin, baud
```

## API

### Methods

- `UART_MIDI.new(tx_pin, rx_pin, baud = 0)` - Initialize UART MIDI; baud
  defaults to 31250 (standard MIDI DIN) when 0 or omitted
- `transport_id` - Returns 2 (`MIDI_TRANSPORT_ID_SERIAL`); used by
  `picoruby-midi` for transport-mask dispatch
- `send_packet(cable, cin, midi1, midi2, midi3)` - Send one USB-MIDI
  format 4-byte packet (cable is ignored — UART has a single channel)
- `connected?` - Returns true once the UART driver is initialized
- `status` - `NOT_INITIALIZED`, `READY`, or `ERROR`
- `device_info` - Hash with `:baud_rate`, `:uart_num`, `:tx_pin`, `:rx_pin`
- `start_input` / `stop_input` / `input_running?` - Control the RX
  background task (no-op if `rx_pin == -1`)

## Notes

- Single-instance for now: one UART_MIDI port per build. Multi-port
  support is a future enhancement.
- The CIN constants (`CIN_NOTE_ON`, `CIN_NOTE_OFF`, ...) match USB-MIDI
  1.0 numeric values so packets can be passed unchanged from
  `picoruby-midi`'s scheduler / clock paths.
- ESP32 port: defaults to `UART_NUM_2`. Override at build time with
  `-DUART_MIDI_UART_NUM=...` if it conflicts with another peripheral.
