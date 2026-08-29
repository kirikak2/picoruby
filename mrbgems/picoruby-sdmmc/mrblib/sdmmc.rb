class SDMMC
  # SDMMC bus width modes
  WIDTH_1BIT = 1
  WIDTH_4BIT = 4

  # Let the port pick the host slot / bus clock
  AUTO = -1

  # Pin accessors are defined in C (src/mrubyc/sdmmc.c)
  # clk_pin, cmd_pin, d0_pin, d1_pin, d2_pin, d3_pin, width, slot, freq_khz

  # @param slot [Integer] host slot the card is wired to, or AUTO to let the
  #   port choose. Boards are wired to one specific slot and the pin numbers
  #   do not always say which -- on the ESP32-P4 the slot 0 pins are
  #   IOMUX-only, so a card on those pins is unreachable through slot 1.
  # @param freq_khz [Integer] bus clock ceiling, or AUTO for the port default
  def self.new(clk_pin:, cmd_pin:, d0_pin:, d1_pin: -1, d2_pin: -1, d3_pin: -1,
               width: WIDTH_1BIT, slot: AUTO, freq_khz: AUTO)
    sdmmc = self._init(clk_pin, cmd_pin, d0_pin, d1_pin, d2_pin, d3_pin, width,
                       slot, freq_khz)
    sdmmc
  end
end
