#include <Adafruit_SI1145.h>
#include <BLE_API.h>

//#define DEBUG 1

BLE                                       ble;
Timeout                                   timeout;
Ticker                                    uv_ticker;
Ticker                                    ir_ticker;
Ticker                                    hi_ticker;

Adafruit_SI1145 uv_dev = Adafruit_SI1145();

// The Nordic UART Service
static const uint8_t service1_uuid[]                = {0x71, 0x3D, 0, 0, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t service1_uv_uuid[]             = {0x71, 0x3D, 0, 4, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t service1_ir_uuid[]             = {0x71, 0x3D, 0, 3, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t service1_hi_uuid[]             = {0x71, 0x3D, 0, 2, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};

static const uint8_t uart_base_uuid_rev[]           = {0x1E, 0x94, 0x8D, 0xF1, 0x48, 0x31, 0x94, 0xBA, 0x75, 0x4C, 0x3E, 0x50, 0, 0, 0x3D, 0x71};

uint16_t uv_value[1] = {0};
uint16_t ir_value[1] = {0};
uint16_t hi_value[1] = {0};


GattCharacteristic  characteristic1(service1_uv_uuid, (uint8_t*)uv_value, 1, 2, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);

GattCharacteristic  characteristic2(service1_ir_uuid, (uint8_t*)ir_value, 1, 2, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);

GattCharacteristic  characteristic3(service1_hi_uuid, (uint8_t*)hi_value, 1, 2, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);

GattCharacteristic *sensorChars[] = {&characteristic1, &characteristic2, &characteristic3};

GattService         uartService(service1_uuid, sensorChars, sizeof(sensorChars) / sizeof(GattCharacteristic *));



void m_uv_ticker_handle(void)
{
  uv_value[0] = uv_dev.readUV();
  float UVindex = (float)uv_value[0];
  UVindex /= 100.0;
  #ifdef DEBUG
  Serial.print("UV:");  Serial.println(UVindex);
  #endif    
  m_uv_handle();
}

void m_ir_ticker_handle(void)
{
  ir_value[0] = uv_dev.readIR();
  #ifdef DEBUG
  Serial.print("IR:"); Serial.print(ir_value[0]);
  #endif    
  m_ir_handle();
}

void m_hi_ticker_handle(void)
{
  hi_value[0] = uv_dev.readVisible();
  #ifdef DEBUG
  Serial.print("Vis, "); Serial.print(hi_value[0]);
  #endif    
  m_hi_handle();
}

void disconnectionCallBack(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    ble.startAdvertising();
}

void m_uv_handle()
{   //update characteristic data
    ble.updateCharacteristicValue(characteristic1.getValueAttribute().getHandle(), (uint8_t*)uv_value, 2);
}

void m_ir_handle()
{   //update characteristic data
    ble.updateCharacteristicValue(characteristic2.getValueAttribute().getHandle(), (uint8_t*)ir_value, 2);
}

void m_hi_handle()
{   //update characteristic data
    ble.updateCharacteristicValue(characteristic3.getValueAttribute().getHandle(), (uint8_t*)hi_value, 2);
}

void setup() {

    // put your setup code here, to run once
    #ifdef DEBUG
    Serial.begin(9600);
    #endif

    ble.init();
    ble.onDisconnection(disconnectionCallBack);

    // setup adv_data and srp_data
    ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                     (const uint8_t *)"UV", sizeof("UV") - 1);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                     (const uint8_t *)uart_base_uuid_rev, sizeof(uart_base_uuid_rev));

    // set adv_type
    ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    // add service
    ble.addService(uartService);
    // set device name
    ble.setDeviceName((const uint8_t *)"PAC_UV");
    // set tx power,valid values are -40, -20, -16, -12, -8, -4, 0, 4
    ble.setTxPower(4);
    // set adv_interval, 100ms in multiples of 0.625ms.
    ble.setAdvertisingInterval(160*10);
    // set adv_timeout, in seconds
    ble.setAdvertisingTimeout(0);
    // start advertising
    ble.startAdvertising();
    if (! uv_dev.begin())
    {
      #ifdef DEBUG
      Serial.println("Didn't find Si1145");
      #endif    
    }
    uv_ticker.attach_us(m_uv_ticker_handle, 1000000);
    hi_ticker.attach_us(m_hi_ticker_handle, 1000000);
    ir_ticker.attach_us(m_ir_ticker_handle, 1000000);

}

void loop()
{
    ble.waitForEvent();
}
