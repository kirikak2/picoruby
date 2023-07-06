require 'adc'

class MyPeripheral < BLE::Peripheral
  # for advertising
  APP_AD_FLAGS = 0x06
  BLUETOOTH_DATA_TYPE_FLAGS = 0x01
  BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS = 0x03
  BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME = 0x09
  # for GATT
  GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION = 0x01
  BTSTACK_EVENT_STATE = 0x60
  HCI_EVENT_DISCONNECTION_COMPLETE = 0x05
  ATT_EVENT_CAN_SEND_NOW = 0xB7
  ATT_EVENT_DISCONNECTED = 0xB4
  ATT_EVENT_MTU_EXCHANGE_COMPLETE = 0xB5
  #
  SERVICE_ENVIRONMENTAL_SENSING = 0x181A
  CHARACTERISTIC_TEMPERATURE = 0x2A6E

  def initialize(debug)
    db = BLE::GattDatabase.new do |db|
      db.add_service(BLE::GATT_PRIMARY_SERVICE_UUID, BLE::GAP_SERVICE_UUID) do |s|
        s.add_characteristic(BLE::GAP_DEVICE_NAME_UUID, BLE::READ, "picoR_temp")
      end
      db.add_service(BLE::GATT_PRIMARY_SERVICE_UUID, BLE::GATT_SERVICE_UUID) do |s|
        s.add_characteristic(BLE::CHARACTERISTIC_DATABASE_HASH, BLE::READ)
      end
      db.add_service(BLE::GATT_PRIMARY_SERVICE_UUID, SERVICE_ENVIRONMENTAL_SENSING) do |s|
        s.add_characteristic(CHARACTERISTIC_TEMPERATURE, BLE::READ|BLE::NOTIFY|BLE::INDICATE|BLE::DYNAMIC)
      end
    end
    @temperature_handle = db.handle_table[:characteristic][:value][CHARACTERISTIC_TEMPERATURE]
    @configuration_handle = db.handle_table[:characteristic][:client_configuration][CHARACTERISTIC_TEMPERATURE]
    super(db.profile_data, debug)
    @last_event = 0
    @led = CYW43::GPIO.new(CYW43::GPIO::LED_PIN)
    @led_on = false
    @counter = 0
    @adv_data = BLE::AdvertisingData.build do |a|
      a.add(BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS)
      a.add(BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, "PicoRuby BLE")
      a.add(BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x181a)
    end
    @adc = ADC.new(:temperature)
  end

  def heartbeat_callback
    @counter += 1
    temperature = ((27 - (@adc.read * 3.3 / (1<<12) - 0.706) / 0.001721) * 100).to_i
    set_read_value(@temperature_handle, BLE::Utils.int16_to_little_endian(temperature))
    if @counter == 10
      if @notification_enabled
        debug_puts "request_can_send_now_event"
        request_can_send_now_event
      end
      @counter = 0
    end
    case @last_event
    when BTSTACK_EVENT_STATE, HCI_EVENT_DISCONNECTION_COMPLETE, ATT_EVENT_DISCONNECTED
      @led_on = !@led_on
      @led.write(@led_on ? 1 : 0)
    end
    if write_value = get_write_value(@configuration_handle)
      if write_value == "\x01\x00"
        @notification_enabled = true
      else
        @notification_enabled = false
      end
    end
  end

  def packet_callback(event_type)
    debug_puts "event type: #{sprintf "%02X", event_type}"
    @last_event = event_type
    case event_type
    when BTSTACK_EVENT_STATE
      debug_puts "Peripheral is up and running on: `#{BLE::Utils.bd_addr_to_str(gap_local_bd_addr)}`"
      advertise(@adv_data)
    when HCI_EVENT_DISCONNECTION_COMPLETE, ATT_EVENT_DISCONNECTED
      debug_puts "disconnected"
      @notification_enabled = false
    when ATT_EVENT_MTU_EXCHANGE_COMPLETE
      debug_puts "mtu exchange complete"
      @led.write(0)
    when ATT_EVENT_CAN_SEND_NOW
      @led.write(1)
      notify @temperature_handle
      sleep_ms 10
      @led.write(0)
    end
  end
end

