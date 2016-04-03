#include <Adafruit_SI1145.h>
#include <BLE_API.h>

#define DEBUG 1

BLE                                       ble;
Timeout                                   timeout;
Ticker                                    uv_ticker;
float                                     all;
bool                                      led_on = false;

const int                                 ledPin = 9;      // the pin that the LED is attached to

Adafruit_SI1145 uv_dev = Adafruit_SI1145();

// The Nordic UART Service
static const uint8_t service1_uuid[]                = {0x71, 0x3D, 0, 0, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};
static const uint8_t service1_uv_uuid[]             = {0x71, 0x3D, 0, 4, 0x50, 0x3E, 0x4C, 0x75, 0xBA, 0x94, 0x31, 0x48, 0xF1, 0x8D, 0x94, 0x1E};

static const uint8_t uv_base_uuid_rev[]             = {0x1E, 0x94, 0x8D, 0xF1, 0x48, 0x31, 0x94, 0xBA, 0x75, 0x4C, 0x3E, 0x50, 0, 0, 0x3D, 0x71};

uint16_t uv_value[3] = {0,0,0};

GattCharacteristic  characteristic1(service1_uv_uuid, (uint8_t*)uv_value, 1, 6, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ);

GattCharacteristic *sensorChars[] = {&characteristic1};

GattService         uvService(service1_uuid, sensorChars, sizeof(sensorChars) / sizeof(GattCharacteristic *));

bool connected = false;

void m_uv_handle()
{   //update characteristic data
    ble.updateCharacteristicValue(characteristic1.getValueAttribute().getHandle(), (uint8_t*)uv_value, 6);
}

void m_led_handle()
{

  byte brightness=0;
  
  if (all < 1.0)
  {
    brightness = 125;
  } else if (all < 10.0)
  {
    brightness = 255;
  } else if (all < 40.0) {
    if (led_on) 
    {
          brightness = 255;
          led_on=false;
    } else {
          brightness = 0;
          led_on= true;
    }
  }

  Serial.println("\n\rLED:");
  Serial.println(brightness);

  // set the brightness of the LED:
  analogWrite(ledPin, brightness);
}

void m_uv_ticker_handle(void)
{
  uv_value[0] = uv_dev.readUV();
  float UVindex = (float)uv_value[0];
  UVindex /= 100.0;
  #ifdef DEBUG
  Serial.print("\n\rUV:");  Serial.println(UVindex);
  #endif    
}

void m_ir_ticker_handle(void)
{
  uv_value[1] = uv_dev.readIR();
  #ifdef DEBUG
  Serial.print("\n\rIR:"); Serial.print(uv_value[1]);
  #endif    
}


void m_hi_ticker_handle(void)
{
  all = uv_dev.readVisible();
  uv_value[2] = (uint16_t)all;
  #ifdef DEBUG
  Serial.print("\n\rVis:"); Serial.print(all);
  #endif    
}

void disconnectionCallBack(Gap::Handle_t handle, Gap::DisconnectionReason_t reason)
{
    connected = false;
    ble.startAdvertising();
}

void connectionCallBack(const Gap::ConnectionCallbackParams_t *p_conn_param)
{
    connected = true;
    ble.stopAdvertising();
}


int v_index =0;

void m_ticker_handle()
{
  if (v_index==0) {
    m_uv_ticker_handle();
    v_index++;
  } else if (v_index==1) {
    m_ir_ticker_handle();
    v_index++;
  } else if (v_index==2) {
    m_hi_ticker_handle();
    v_index++;
    if (!connected) return;
    m_uv_handle();
  } else if (v_index==3) 
  {
    m_led_handle();
    v_index=0;
  }
}

void setup() {

    // put your setup code here, to run once
    #ifdef DEBUG
    Serial.begin(9600);
    #endif

    ble.init();
    ble.onConnection(connectionCallBack);
    ble.onDisconnection(disconnectionCallBack);

    // setup adv_data and srp_data
    ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                     (const uint8_t *)"UV", sizeof("UV") - 1);
    ble.accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                     (const uint8_t *)uv_base_uuid_rev, sizeof(uv_base_uuid_rev));

    // set adv_type
    ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    // add service
    ble.addService(uvService);
    // set device name
    ble.setDeviceName((const uint8_t *)"UV");
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
    uv_ticker.attach_us(m_ticker_handle,    100000);

    // initialize the ledPin as an output:
    pinMode(ledPin, OUTPUT);


}

void loop()
{
    ble.waitForEvent();
}
