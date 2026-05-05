# UART/Serial MIDI transport layer
#
# Generic UART-based MIDI transport for PicoRuby. Defaults to standard
# 5-pin MIDI DIN (31250 baud, 8N1); the third arg lets you override the
# baud for chips like the SAM2695 that speak MIDI over a non-standard
# rate. Use picoruby-midi for high-level MIDI operations.
#
# Usage:
#   require 'uart_midi'
#   uart = UART_MIDI.new(13, -1)         # MIDI DIN: 31250 baud, TX-only
#   uart = UART_MIDI.new(17, 18)         # MIDI DIN with RX
#   uart = UART_MIDI.new(13, -1, 38400)  # custom baud (e.g. SAM2695)
#   device = MIDI::Device.new(uart)
#   device.note_on(60, 100)
#
class UART_MIDI
  # Initialize UART MIDI interface.
  # @param tx_pin [Integer] GPIO pin for MIDI TX (required)
  # @param rx_pin [Integer] GPIO pin for MIDI RX (-1 to disable)
  # @param baud   [Integer] UART baud (0 or omitted -> 31250)
  def initialize(tx_pin, rx_pin, baud = 0)
    @tx_pin = tx_pin
    @rx_pin = rx_pin
    @baud   = (baud == 0) ? DEFAULT_BAUD_RATE : baud
    _init(tx_pin, rx_pin, baud)
  end

  # Identifier consumed by picoruby-midi's transport-mask dispatch.
  # Matches MIDI_TRANSPORT_ID_SERIAL.
  def transport_id
    2
  end

  def tx_pin; @tx_pin; end
  def rx_pin; @rx_pin; end
  def baud;   @baud;   end

  def status
    _get_status
  end

  def connected?
    status == READY
  end

  def device_info
    return nil unless connected?
    _get_device_info
  end

  def open_device
    return nil unless connected?
    self
  end

  # Send a USB-MIDI format packet. The cable parameter is ignored
  # (UART is a single channel).
  def send_packet(cable, cin, midi1, midi2, midi3)
    _send_packet(cable, cin, midi1, midi2, midi3)
  end

  def start_input
    return -1 if @rx_pin < 0
    _input_start
  end

  def stop_input
    _input_stop
  end

  def input_running?
    _input_is_running
  end

  # MIDI input is consumed by the C-side midi_input_task and dispatched
  # through MIDI::Input's event system, so these are stubs for protocol
  # compatibility with the transport interface.
  def bytes_available
    0
  end

  def read_available
    nil
  end
end
