#include <SoftwareSerial.h>
#include "Arduino.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "PCF8574.h"
// RE must be connected to DE Pin and to D5
// D1 - GPIO5 - SCL
// D2 - GPIO4 - SDA

#define MOSFET_PIN D3  // MOSFET to toggle ON/OFF the expander module

#define RXPin D6  // Serial Receive pin //D2 also works
#define TXPin D7  // Serial Transmit pin //D3 also works

//RS485 control
#define SERIAL_COMMUNICATION_CONTROL_PIN D5  // Transmission set pin //D0 also works
#define RS485_TX_PIN_VALUE HIGH
#define RS485_RX_PIN_VALUE LOW

SoftwareSerial RS485Serial(RXPin, TXPin);  // RX, TX
Adafruit_BME680 bme;                       // I2C
// Set the i2c HEX address
PCF8574 pcf8574(0x20);

// Modbus RTU requests for reading NPK values
const byte nitro[] = { 0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c };
const byte phos[] = { 0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc };
const byte pota[] = { 0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0 };

// A variable to hold soil NPK values
byte values[11];

// Variables to hold air sensor values
float temperature;
float humidity;
float pressure;
float gasResistance;


void setup() {
  // Set the baud rate for the Serial port
  Serial.begin(9600);
  while (!Serial) delay(10);  // wait for console
  pinMode(SERIAL_COMMUNICATION_CONTROL_PIN, OUTPUT);
  digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_RX_PIN_VALUE);
  RS485Serial.begin(9600);  // set the data rate
  delay(500);
  // Set the pinMode for MOSFET_PIN
  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, HIGH);  // Initially keep the MOSFET off
  // Setting the expander's pinModes
  pcf8574.pinMode(P0, OUTPUT);
  pcf8574.pinMode(P1, OUTPUT);
  pcf8574.pinMode(P2, OUTPUT);
  pcf8574.pinMode(P3, OUTPUT);
  pcf8574.pinMode(P4, OUTPUT);
  pcf8574.pinMode(P5, OUTPUT);
  pcf8574.pinMode(P6, OUTPUT);
  pcf8574.pinMode(P7, OUTPUT);

  pcf8574.begin();
  bme.begin();

  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    while (1)
      ;
  }


  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);  // 320*C for 150 ms
}

void loop() {

  // Turning on power to the expander (PCF8574RGTR)
  digitalWrite(MOSFET_PIN, HIGH);  // Turn off MOSFET for safety
  delay(100);                      // Wait for the expander to stabilize
  digitalWrite(MOSFET_PIN, LOW);   // Turn on MOSFET to power the expander
  delay(2000);

  //Expander related gpio control operations here

  // //LTE_RST
  // pcf8574.digitalWrite(P0, HIGH);  //HIGH WILL SWITCH on
  // delay(1000);
  // pcf8574.digitalWrite(P0, LOW);

  // //LAT_4 = FREE
  // pcf8574.digitalWrite(P1, HIGH);  //HIGH WILL SWITCH on
  // delay(1000);
  // pcf8574.digitalWrite(P1, LOW);

  // //LAT_2 = HAT-PWR & BATT GAUGE
  // pcf8574.digitalWrite(P2, HIGH);  //HIGH WILL SWITCH on
  // delay(1000);
  // pcf8574.digitalWrite(P2, LOW);

  // //LAT_3 = LORA RESET (high) & sim7000 PWRKEY (high)
  // pcf8574.digitalWrite(P3, HIGH);  //HIGH WILL SWITCH on
  // delay(1000);
  // pcf8574.digitalWrite(P3, LOW);

  // //LTE_DTR = READY TO TRANSMIT
  // pcf8574.digitalWrite(P4, HIGH);  //HIGH WILL SWITCH on
  // delay(1000);
  // pcf8574.digitalWrite(P4, LOW);

  // //LORA_M1 =
  // pcf8574.digitalWrite(P5, HIGH);  //HIGH WILL SWITCH on
  // delay(1000);
  // pcf8574.digitalWrite(P5, LOW);

  // //LORA_M0 =
  // pcf8574.digitalWrite(P6, HIGH);  //HIGH WILL SWITCH on
  // delay(1000);
  // pcf8574.digitalWrite(P6, LOW);

  // //LORA_AUX =
  // pcf8574.digitalWrite(P7, HIGH);  //HIGH WILL SWITCH on
  // delay(1000);
  // pcf8574.digitalWrite(P7, LOW);

  //Forward loop
  for (int pin = P0; pin <= P7; pin++) {
    pcf8574.digitalWrite(pin, HIGH);
    delay(1000);
    pcf8574.digitalWrite(pin, LOW);
  }
  // Reverse loop
  for (int pin = P7; pin >= P0; pin--) {
    pcf8574.digitalWrite(pin, HIGH);
    delay(1000);
    pcf8574.digitalWrite(pin, LOW);
  }

  digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_RX_PIN_VALUE);
  // Read values
  byte val1, val2, val3;
  val1 = nitrogen();
  delay(250);
  val2 = phosphorous();
  delay(250);
  val3 = potassium();
  delay(250);

  // Print soil values to the serial monitor
  Serial.print("Nitrogen: ");
  Serial.print(val1);
  Serial.println(" mg/kg");
  Serial.print("Phosphorous: ");
  Serial.print(val2);
  Serial.println(" mg/kg");
  Serial.print("Potassium: ");
  Serial.print(val3);
  Serial.println(" mg/kg");
  // Print air values to the serial monitor
  getBME680Readings();
  Serial.println();
  Serial.printf("Temperature = %.2f ºC \n", temperature);
  Serial.printf("Humidity = %.2f % \n", humidity);
  Serial.printf("Pressure = %.2f hPa \n", pressure);
  Serial.printf("Gas Resistance = %.2f KOhm \n", gasResistance);

  delay(2000);

  // Turning off power to the expander after the loop
  digitalWrite(MOSFET_PIN, HIGH);  // Turn off MOSFET to cut power to the expander
  delay(500);                      // Wait before turning off
}

byte nitrogen() {
  // digitalWrite(DE, HIGH);
  // digitalWrite(RE, HIGH);
  digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_TX_PIN_VALUE);
  delay(10);
  if (RS485Serial.write(nitro, sizeof(nitro)) == 8) {
    // digitalWrite(DE, LOW);
    // digitalWrite(RE, LOW);
    digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_RX_PIN_VALUE);
    for (byte i = 0; i < 7; i++) {
      //Serial.print(mod.read(),HEX);
      values[i] = RS485Serial.read();
      // Serial.print(values[i], HEX);
    }
    Serial.println();
  }
  return values[4];
}

byte phosphorous() {
  // digitalWrite(DE, HIGH);
  // digitalWrite(RE, HIGH);
  digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_TX_PIN_VALUE);
  delay(10);
  if (RS485Serial.write(phos, sizeof(phos)) == 8) {
    // digitalWrite(DE, LOW);
    // digitalWrite(RE, LOW);
    digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_RX_PIN_VALUE);
    for (byte i = 0; i < 7; i++) {
      //Serial.print(mod.read(),HEX);
      values[i] = RS485Serial.read();
      // Serial.print(values[i], HEX);
    }
    Serial.println();
  }
  return values[4];
}

byte potassium() {
  // digitalWrite(DE, HIGH);
  // digitalWrite(RE, HIGH);
  digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_TX_PIN_VALUE);
  delay(10);
  if (RS485Serial.write(pota, sizeof(pota)) == 8) {
    // digitalWrite(DE, LOW);
    // digitalWrite(RE, LOW);
    digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_RX_PIN_VALUE);
    for (byte i = 0; i < 7; i++) {
      //Serial.print(mod.read(),HEX);
      values[i] = RS485Serial.read();
      // Serial.print(values[i], HEX);
    }
    Serial.println();
  }
  return values[4];
}

void getBME680Readings() {
  // Tell BME680 to begin measurement.
  unsigned long endTime = bme.beginReading();
  if (endTime == 0) {
    Serial.println(F("Failed to begin reading :("));
    return;
  }
  if (!bme.endReading()) {
    Serial.println(F("Failed to complete reading :("));
    return;
  }
  temperature = bme.temperature;
  pressure = bme.pressure / 100.0;
  humidity = bme.humidity;
  gasResistance = bme.gas_resistance / 1000.0;
}
