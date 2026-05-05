# Forward Note On/Off events from a connected USB MIDI device to stdout.
# Plug in any USB MIDI controller (keyboard, drum pad, etc.) and play.

require 'midi'
require 'usb_midi_host'

usb = USB_MIDI_HOST.instance
puts "Waiting for USB MIDI device..."
sleep_ms 100 until usb.connected?

info = usb.device_info
puts "Connected: #{info[:manufacturer]} #{info[:product]}"

device = MIDI::Device.new(usb)
input = MIDI::Input.new(device)

input.on(:note_on)  { |e| puts "ON  ch=#{e[:channel]} note=#{e[:note]} vel=#{e[:velocity]}" }
input.on(:note_off) { |e| puts "OFF ch=#{e[:channel]} note=#{e[:note]}" }

input.start
loop do
  input.process
  sleep_ms 5
end
