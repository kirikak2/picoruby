# Play a C-major chord on a MIDI-DIN device wired to UART pins 17 (TX)
# and 18 (RX). Adjust pins for your board.

require 'midi'
require 'uart_midi'

uart = UART_MIDI.new(17, 18)
device = MIDI::Device.new(uart)

# C major chord: C4, E4, G4
[60, 64, 67].each { |note| device.note_on(note, 100) }
sleep 1
[60, 64, 67].each { |note| device.note_off(note) }
