# USB-MIDI Host transport layer
#
# Provides low-level USB MIDI communication for PicoRuby.
# Use picoruby-midi for high-level MIDI operations.
#
class USB_MIDI_HOST
  # Get singleton instance
  def self.instance
    $__usb_midi_host_instance__ = new if $__usb_midi_host_instance__.nil?
    $__usb_midi_host_instance__
  end

  # Initialize USB MIDI subsystem
  def initialize
    _init
  end

  # Identifier consumed by picoruby-midi's transport-mask dispatch.
  # Matches MIDI_TRANSPORT_USB and MIDI_TRANSPORT_ID_USB.
  def transport_id
    1
  end

  # Get current connection status
  # Returns: DISCONNECTED, CONNECTED, INITIALIZING, or ERROR
  def status
    _get_status
  end

  # Check if a MIDI device is connected
  def connected?
    status == CONNECTED
  end

  # Get connected device information
  # Returns Hash with keys:
  #   :vendor_id, :product_id, :manufacturer, :product,
  #   :midi_in_ep, :midi_out_ep
  # Returns nil if not connected
  def device_info
    return nil unless connected?
    _get_device_info
  end

  # Open a MIDI device for use with picoruby-midi
  # Returns self as transport handle
  def open_device
    return nil unless connected?
    self
  end

  # Send a USB-MIDI packet
  # @param cable [Integer] Cable number (0-15)
  # @param cin [Integer] Code Index Number (see CIN_* constants)
  # @param midi1 [Integer] First MIDI byte (status)
  # @param midi2 [Integer] Second MIDI byte (data 1)
  # @param midi3 [Integer] Third MIDI byte (data 2)
  # @return [Integer] 0 on success, -1 on error
  def send_packet(cable, cin, midi1, midi2, midi3)
    _send_packet(cable, cin, midi1, midi2, midi3)
  end

  # Get number of bytes available in receive buffer
  def bytes_available
    _bytes_available
  end

  # Read available MIDI packets
  # Returns binary String containing USB-MIDI packets (4 bytes each)
  # Returns nil if no data available
  def read_available
    _read_available
  end
end
