class BLE
  class AdvertisingReport
    EVENT_TYPE = {
      -1 => :unknown,
      0x00 => :connectable_advertising_ind,
      0x01 => :flags,
      0x02 => :incomplete_list_16_bit_service_class_uuids,
      0x03 => :complete_list_16_bit_service_class_uuids,
      0x04 => :incomplete_list_32_bit_service_class_uuids,
      0x05 => :complete_list_32_bit_service_class_uuids,
      0x06 => :incomplete_list_128_bit_service_class_uuids,
      0x07 => :complete_list_128_bit_service_class_uuids,
      0x08 => :shortened_local_name,
      0x09 => :complete_local_name,
      0x0A => :tx_power_level,
      0xff => :manufacturer_specific_data
    }
    ADDRESS_TYPE = [
      :public,
      :random,
      :public_identity,
      :random_identity
    ]
    attr_reader :event_type, :address_type, :address, :rssi, :reports

    def initialize(packet)
      if packet.length < 14
        raise ArgumentError, "packet length must be 14 or more"
      end
      @event_type = packet[2]&.ord || -1
      @address_type = packet[3]&.ord || 4
      @address = ""
      5.downto(0) do |i|
        @address << (packet[4 + i] || "\x00")
      end
      @rssi = (packet[10]&.ord || 0) - 256
      data_length = packet[11]&.ord || 0
      @reports = inspect_reports(packet[12, data_length] || "")
    end

    def format
      "Event Type: #{EVENT_TYPE[@event_type] || @event_type&.to_s(16)}" +
      "\nAddress Type: #{ADDRESS_TYPE[@address_type] || @address_type&.to_s(16)}" +
      "\nAddress: #{BLE::Utils.bd_addr_to_str(@address)}" +
      "\nRSSI: #{@rssi}" +
      "\nReports:\n" +
      @reports.map{|d| "  #{EVENT_TYPE[d[:type]] || 'N/A'}: #{d[:value].inspect}(len #{d[:value].length})"}.join("\n")
    end

    def name_include?(name)
      return false unless name
      @reports.each do |report|
        if report[:type] == 0x09 || report[:type] == 0x08
          return true if report[:value].include?(name)
        end
      end
      false
    end

    # private

    def inspect_reports(data)
      reports = []
      index = 0
      while index < data.length
        length = data[index]&.ord || 0
        break if length.nil?
        type = data[index + 1]&.ord || -1
        break if type.nil?
        value = data[index + 2, length - 1]
        break if value.nil?
        reports << {type: type, value: value}
        index += length + 1
      end
      reports
    end

  end
end