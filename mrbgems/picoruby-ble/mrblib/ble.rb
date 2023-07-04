require 'mbedtls'
require 'cyw43'

class BLE
  # GATT Characteristic Properties
  BROADCAST =                   0x01
  READ =                        0x02
  WRITE_WITHOUT_RESPONSE =      0x04
  WRITE =                       0x08
  NOTIFY =                      0x10
  INDICATE =                    0x20
  AUTHENTICATED_SIGNED_WRITE =  0x40
  EXTENDED_PROPERTIES =         0x80
  # custom BTstack extension
  DYNAMIC =                     0x100
  LONG_UUID =                   0x200

  # read permissions
  READ_PERMISSION_BIT_0 =       0x400
  READ_PERMISSION_BIT_1 =       0x800

  ENCRYPTION_KEY_SIZE_7 =       0x6000
  ENCRYPTION_KEY_SIZE_8 =       0x7000
  ENCRYPTION_KEY_SIZE_9 =       0x8000
  ENCRYPTION_KEY_SIZE_10 =      0x9000
  ENCRYPTION_KEY_SIZE_11 =      0xa000
  ENCRYPTION_KEY_SIZE_12 =      0xb000
  ENCRYPTION_KEY_SIZE_13 =      0xc000
  ENCRYPTION_KEY_SIZE_14 =      0xd000
  ENCRYPTION_KEY_SIZE_15 =      0xe000
  ENCRYPTION_KEY_SIZE_16 =      0xf000
  ENCRYPTION_KEY_SIZE_MASK =    0xf000

  # only used by gatt compiler >= 0xffff
  # Extended Properties
  RELIABLE_WRITE =              0x00010000
  AUTHENTICATION_REQUIRED =     0x00020000
  AUTHORIZATION_REQUIRED =      0x00040000
  READ_ANYBODY =                0x00080000
  READ_ENCRYPTED =              0x00100000
  READ_AUTHENTICATED =          0x00200000
  READ_AUTHENTICATED_SC =       0x00400000
  READ_AUTHORIZED =             0x00800000
  WRITE_ANYBODY =               0x01000000
  WRITE_ENCRYPTED =             0x02000000
  WRITE_AUTHENTICATED =         0x04000000
  WRITE_AUTHENTICATED_SC =      0x08000000
  WRITE_AUTHORIZED =            0x10000000

  # Broadcast, Notify, Indicate, Extended Properties are only used to describe a GATT Characteristic, but are free to use with att_db
  # - write permissions
  WRITE_PERMISSION_BIT_0 =      0x01
  WRITE_PERMISSION_BIT_1 =      0x10
  # - SC required
  READ_PERMISSION_SC =          0x20
  WRITE_PERMISSION_SC =         0x80

  GAP_DEVICE_NAME_UUID = 0x2a00
  GATT_PRIMARY_SERVICE_UUID = 0x2800
  GATT_SECONDARY_SERVICE_UUID = 0x2801
  GATT_CHARACTERISTIC_UUID = 0x2803
  GAP_SERVICE_UUID = 0x1800
  GATT_SERVICE_UUID = 0x1801
  CLIENT_CHARACTERISTIC_CONFIGURATION = 0x2902
  CHARACTERISTIC_DATABASE_HASH = 0x2b2a

end

