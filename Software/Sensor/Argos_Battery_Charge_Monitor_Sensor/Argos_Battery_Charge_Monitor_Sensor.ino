#include <TinyWireS.h>

#define FIRST_I2C_ADDR 0b010000
#define MIN_CHARGE_CURRENT 0.05

#define REV2017A // Sensor version

#ifdef REV2017A
  #define PIN_LED 3
  #define PIN_ADDR_0 7
  #define PIN_ADDR_1 8
  #define PIN_ADDR_2 9
  #define PIN_ADDR_3 10
  #define PIN_CURRENT A1
  #define PIN_VOLTAGE A0

  #define LED_PWM 0

  #define SCALE_VOLTAGE  0.0195 // V/bit
  #define ZERO_VOLTAGE_VAL 0.0
  #define SCALE_CURRENT  0.0488 // A/bit
  #define ZERO_CURRENT_VAL 512.0
#endif

#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE ( 16 )
#endif

uint8_t i2cAddr = 0;

volatile uint8_t i2cRegs[] =
{
  0x00, // unused
  0x01, // current[0]
  0x02, // current[1]
  0x03, // current[2]
  0x04, // current[3]
  0x05, // voltage[0]
  0x06, // voltage[1]
  0x07, // voltage[2]
  0x08  // voltage[3]
};

#define CURRENT_REG_START 1
#define VOLTAGE_REG_START 5

volatile uint8_t i2cRegIdx = 0;
const uint8_t i2cNumReg = sizeof(i2cRegs);

void flashNumber( const uint8_t pin, const uint8_t val )
{
  digitalWrite(pin, LOW);
  for(uint8_t i = 0; i < val; i++)
  {
    digitalWrite(pin, HIGH);
    delay(250);
    digitalWrite(pin, LOW);
    delay(250);
  }
  delay(500);
}

void setup()
{
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_ADDR_0, INPUT_PULLUP);
  pinMode(PIN_ADDR_1, INPUT_PULLUP);
  pinMode(PIN_ADDR_2, INPUT_PULLUP);
  pinMode(PIN_ADDR_3, INPUT_PULLUP);

  // Wait for address value
  delay(500); // 500ms

  // Read I2C address pins
  i2cAddr += !digitalRead(PIN_ADDR_3);
  i2cAddr = i2cAddr << 1;
  i2cAddr += !digitalRead(PIN_ADDR_2);
  i2cAddr = i2cAddr << 1;
  i2cAddr += !digitalRead(PIN_ADDR_1);
  i2cAddr = i2cAddr << 1;
  i2cAddr += !digitalRead(PIN_ADDR_0);

  flashNumber(PIN_LED, i2cAddr);

  // Offset to base address
  i2cAddr += FIRST_I2C_ADDR;

  // Initialize I2C Slave
  TinyWireS.begin(i2cAddr);
  TinyWireS.onRequest(requestCallback);
  TinyWireS.onReceive(receiveCallback);
}

void loop()
{
  static uint16_t ledCounter = 0;
  static bool ledState = false;
  // put your main code here, to run repeatedly:
  // TinyWireS_stop_check();
  tws_delay(50);
  float voltage = readVoltage();
  float current = readCurrent();

  bool charging = current > MIN_CHARGE_CURRENT;

  // 1 Hz blink
  if(ledCounter % 10 == 0 && true == charging)
  {
    ledState = !ledState;
    digitalWrite(PIN_LED,ledState ? HIGH : LOW);
  }
  else if(false == charging)
  {
    digitalWrite(PIN_LED, HIGH);
  }
  ledCounter++;
}

void requestCallback()
{
  // TinyWireS.send(0);
  if(i2cRegIdx < i2cNumReg)
  {
    TinyWireS.send(i2cRegs[i2cRegIdx]);
  }
  else
  {
    TinyWireS.send(0);
  }
  i2cRegIdx++;
  if(i2cRegIdx >= i2cNumReg)
  {
    i2cRegIdx = 0;
  }
}

void receiveCallback(uint8_t numBytes)
{
  if(numBytes < 1)
  {
    return;
  }
  if(numBytes > TWI_RX_BUFFER_SIZE)
  {
    return;
  }

  i2cRegIdx = TinyWireS.receive();
  numBytes--;

  // Don't support writing to registers right now
  while(numBytes--)
  {
    TinyWireS.receive(); // discard
  }
  return;
}

float readCurrent()
{
  // int cRaw = analogRead(PIN_CURRENT);
  // memcpy(&i2cRegs[CURRENT_REG_START], &cRaw, sizeof(cRaw));
  // for(unsigned int i = CURRENT_REG_START; i < sizeof(cRaw) + CURRENT_REG_START; i++)
  // {
  //   i2cRegs[i] = reinterpret_cast<uint8_t *>(&cRaw)[i-CURRENT_REG_START];
  // }
  float current = ( ( static_cast<float>( analogRead(PIN_CURRENT) ) - ZERO_CURRENT_VAL ) * SCALE_CURRENT );
  memcpy(&i2cRegs[CURRENT_REG_START], &current, sizeof(current));
  // for(unsigned int i = CURRENT_REG_START; i < sizeof(current) + CURRENT_REG_START; i++)
  // {
  //   i2cRegs[i] = reinterpret_cast<uint8_t *>(&current)[i-CURRENT_REG_START];
  // }
  return current;
}

float readVoltage()
{
  // int vRaw = analogRead(PIN_VOLTAGE);
  // memcpy(&i2cRegs[VOLTAGE_REG_START], &vRaw, sizeof(vRaw));
  // for(unsigned int i = VOLTAGE_REG_START; i < sizeof(vRaw) + VOLTAGE_REG_START; i++)
  // {
  //   i2cRegs[i] = reinterpret_cast<uint8_t *>(&vRaw)[i-VOLTAGE_REG_START];
  // }
  float voltage = ( ( static_cast<float>( analogRead(PIN_VOLTAGE) ) - ZERO_VOLTAGE_VAL ) * SCALE_VOLTAGE );
  memcpy(&i2cRegs[VOLTAGE_REG_START], &voltage, sizeof(voltage));
  // for(unsigned int i = VOLTAGE_REG_START; i < sizeof(voltage) + VOLTAGE_REG_START; i++)
  // {
  //   i2cRegs[i] = reinterpret_cast<uint8_t *>(&voltage)[i-VOLTAGE_REG_START];
  // }
  return voltage;
}