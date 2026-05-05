# SAM2695 MIDI Synthesizer
#
# Thin wrapper over UART_MIDI. The SAM2695 is a UART-attached General-MIDI
# synth chip; protocol-wise it is just MIDI DIN at the standard 31250 baud,
# so this gem owns nothing but the device-specific defaults and a place to
# hang future SAM2695-only commands (GS/GM reset, reverb level, ...).
#
# Usage:
#   require 'sam2695'
#   sam = SAM2695.new(17, 18)            # tx_pin, rx_pin
#   device = MIDI::Device.new(sam)
#   device.note_on(60, 100)
#
require 'uart_midi'

class SAM2695
  # SAM2695 speaks standard MIDI baud.
  DEFAULT_BAUD = 31250

  def initialize(tx_pin, rx_pin = -1, baud = DEFAULT_BAUD)
    @uart = UART_MIDI.new(tx_pin, rx_pin, baud)
  end

  # Transport interface — delegated to UART_MIDI.
  def transport_id;        @uart.transport_id;        end
  def send_packet(*args);  @uart.send_packet(*args);  end
  def bytes_available;     @uart.bytes_available;     end
  def read_available;      @uart.read_available;      end
  def connected?;          @uart.connected?;          end
  def open_device;         connected? ? self : nil;   end

  # Pass-throughs that callers of the old SAM2695 surface still expect.
  def status;          @uart.status;            end
  def device_info;     @uart.device_info;       end
  def tx_pin;          @uart.tx_pin;            end
  def rx_pin;          @uart.rx_pin;            end
  def start_input;     @uart.start_input;       end
  def stop_input;      @uart.stop_input;        end
  def input_running?;  @uart.input_running?;    end

  # Future: SAM2695-specific helpers (GS/GM reset, reverb level, ...) live
  # here so that picoruby-uart_midi stays a generic transport.
end
