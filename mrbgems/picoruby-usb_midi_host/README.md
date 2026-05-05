# picoruby-usb_midi_host

USB-MIDI Host transport layer for PicoRuby.

Implements a USB MIDI Host: enumerates attached USB MIDI devices, parses
their interface descriptors, claims the MIDI Streaming subclass, and
exposes the bulk IN/OUT endpoints as a `picoruby-midi` Transport. The
ESP-IDF USB Host driver is bundled inside the gem (see
`ports/esp32/usb_host_driver.c`) so the gem alone is sufficient — the
host application only needs to call the public API.

## Usage

```ruby
require 'midi'
require 'usb_midi_host'

usb = USB_MIDI_HOST.instance
puts "Waiting for USB MIDI device..."
sleep 0.1 until usb.connected?

device = MIDI::Device.new(usb)
device.note_on(60, 100)
sleep 1
device.note_off(60)
```

Receive USB MIDI events:

```ruby
input = MIDI::Input.new(device)
input.on(:note_on)  { |e| puts "note on  #{e[:note]} vel=#{e[:velocity]}" }
input.on(:note_off) { |e| puts "note off #{e[:note]}" }
input.start
loop do
  input.process
  sleep_ms 5
end
```

## API

### Methods

- `USB_MIDI_HOST.instance` - Singleton handle. Lazily initializes the
  USB host stack on first call.
- `transport_id` - Returns 1 (`MIDI_TRANSPORT_ID_USB`)
- `connected?` / `status` - `DISCONNECTED`, `INITIALIZING`, `CONNECTED`,
  `ERROR`
- `device_info` - Hash with `:vendor_id`, `:product_id`,
  `:manufacturer`, `:product`, `:midi_in_ep`, `:midi_out_ep`. Returns
  nil when no device is connected.
- `send_packet(cable, cin, midi1, midi2, midi3)` - Send one USB-MIDI
  4-byte packet
- `open_device` - Returns self once a device is connected, otherwise nil

## Notes

- ESP32 only at present. Other ports (RP2040 + TinyUSB Host, etc.) are
  planned.
- Singleton: only one USB MIDI host instance exists per build.
- Hot-plug aware — `connected?` flips back to false on disconnect, and
  the next plug re-enumerates automatically.
