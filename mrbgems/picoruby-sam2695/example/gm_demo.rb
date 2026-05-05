# Cycle through a few General-MIDI instruments on the SAM2695.
# Wire SAM2695 to UART pins 17 (TX) and 18 (RX); adjust for your board.

require 'midi'
require 'sam2695'

sam = SAM2695.new(17, 18)
device = MIDI::Device.new(sam)

# GM program numbers (1-based in MIDI spec; the gem expects 0-based)
programs = [
  [0,  "Acoustic Grand Piano"],
  [24, "Nylon-String Guitar"],
  [40, "Violin"],
  [56, "Trumpet"],
  [73, "Flute"],
]

programs.each do |program, name|
  puts name
  device.program_change(program, channel: 0)
  [60, 64, 67].each { |n| device.note_on(n, 100) }
  sleep 1
  [60, 64, 67].each { |n| device.note_off(n) }
  sleep 0.2
end
