# picoruby-sam2695

SAM2695 General-MIDI synthesizer transport for PicoRuby — thin wrapper
over `picoruby-uart_midi`.

The SAM2695 is a UART-attached GM synth chip; protocol-wise it is plain
MIDI DIN at 31250 baud, so this gem owns nothing but the device-specific
defaults and a place to hang future SAM2695-only commands (GS/GM reset,
reverb level, ...). Use `picoruby-midi` for high-level MIDI operations.

## Usage

```ruby
require 'midi'
require 'sam2695'

sam = SAM2695.new(17, 18)            # tx_pin, rx_pin
device = MIDI::Device.new(sam)

device.program_change(1, channel: 0)  # Bright Acoustic Piano
device.note_on(60, 100)
sleep 1
device.note_off(60)
```

TX-only (no RX wiring):

```ruby
sam = SAM2695.new(17)                # rx_pin defaults to -1
```

## API

### Methods

- `SAM2695.new(tx_pin, rx_pin = -1, baud = 31250)` - Initialize. The
  baud arg exists for completeness; SAM2695 always uses 31250 in practice.
- `transport_id` - Returns 2 (`MIDI_TRANSPORT_ID_SERIAL`)
- `send_packet(cable, cin, midi1, midi2, midi3)` - Send one USB-MIDI packet
- `connected?` / `status` / `device_info`
- `start_input` / `stop_input` / `input_running?`

All methods are forwarded to the underlying `UART_MIDI` instance.

## Notes

- This gem exists separately from `picoruby-uart_midi` to give SAM2695-
  specific helpers (GS/GM reset, reverb depth, chorus depth, ...) a
  natural home in future revisions.
- See `picoruby-uart_midi` for the underlying transport behavior.
