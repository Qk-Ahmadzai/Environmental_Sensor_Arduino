#include <Arduino.h>
#include <SPI.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

#include <DHT.h> // DHT temperature sensor

#define BUFSIZE         128
#define VERBOSE_MODE    true

// Define Sensor Pins
#define SUNLIGHT_PIN  A2
#define DHT_PIN       A3
#define MOISTURE_PIN  A4

//DHT initalization
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

#define TAKE_AVERAGE_OF_X_READINGS            2
#define SENSOR_READING_DELAY                  100

// Soil Moisture Calibration Values
double Dry=807; //Measured value for air
double Wet=418; //Measured value for water

#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

String data;

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setup() {
  Serial.begin(115200);
      /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );
    if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);
  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }

  

  dht.begin();

}

void loop() {

  ble.println("AT+BLEUARTRX");
  ble.readline();
  if (strcmp(ble.buffer, "OK") == 0) {
    // no data
    return;
  }
  // Some data was found, its in the buffer
  Serial.print(F("[Recv] ")); Serial.println(ble.buffer);
  ble.waitForOK();
  
  double moisture = takeMoistureReading();
  Serial.print("M:");
  Serial.println(moisture);
  int rmoisture = round(moisture);
  
  double sunlight = takeSunlightReading();
  Serial.print("L:");
  Serial.println(sunlight);
  int rsunlight = round(sunlight);
  
  float temp, humidity;
  takeTHReadings(&temp, &humidity);
  Serial.print("T:");
  Serial.println(temp);
  Serial.print("H:");
  Serial.println(humidity);
  
  data = String(rmoisture) + "," + String(rsunlight) + "," + String(temp) + "," + String(humidity);
  
  Serial.print("[Send] ");
  Serial.println(data);
  
  ble.print("AT+BLEUARTTX=");
  ble.println(data);

  // check response stastus
  if (! ble.waitForOK() ) {
    Serial.println(F("Failed to send?"));
  }
}

double takeMoistureReading()
{
  double sumMoisture=0;
  double avgMoisture=0;
  
  for (int i=0; i< TAKE_AVERAGE_OF_X_READINGS; i++)
  {
    sumMoisture += analogRead(MOISTURE_PIN);
    delay(SENSOR_READING_DELAY);
  }

  avgMoisture = 100*((sumMoisture/TAKE_AVERAGE_OF_X_READINGS)-Wet)/(Dry-Wet);
  avgMoisture = 100 - avgMoisture;

  if (avgMoisture < 0 ) {
    avgMoisture = 0;
  }

  if (avgMoisture > 100 ) {
    avgMoisture = 100;
  }

  return avgMoisture;
}

//Average Sunlight of x readings
double takeSunlightReading() {

  double sumSunLight = 0;
  double avgSunLight = 0;

  for (int i = 0; i < TAKE_AVERAGE_OF_X_READINGS; i++)
  {
    //sumSunLight += analogRead(SUNLIGHT_PIN) * 100 / 1000;
    sumSunLight += analogRead(SUNLIGHT_PIN);
    delay(SENSOR_READING_DELAY);
  }

  avgSunLight = (sumSunLight * 100 / 1023) / TAKE_AVERAGE_OF_X_READINGS;
  if (avgSunLight < 0 ) {
    avgSunLight = 0;
  }

  if (avgSunLight > 100 ) {
    avgSunLight = 100;
  }
  return avgSunLight;
}

void takeTHReadings(float *temp, float *humidity){
  float sumh = 0;
  float avgh = 0;
  float sumt = 0;
  float avgt = 0;
  for (int i=0; i<TAKE_AVERAGE_OF_X_READINGS; i++){
    if (i > 0) {
      delay(2000);
      }
    sumh += dht.readHumidity();
    sumt += dht.readTemperature();
  }
  avgh = sumh/TAKE_AVERAGE_OF_X_READINGS;
  avgt = sumt/TAKE_AVERAGE_OF_X_READINGS;
  if (avgh < 0 ) {
    avgh = 0;
  }
  if (avgh > 100) {
    avgh = 100;
  }
  *temp = avgt;
  *humidity = avgh;
}
