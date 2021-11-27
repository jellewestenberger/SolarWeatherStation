
/*********
  Complete project details at http://randomnerdtutorials.com  
*********/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h> 

#include <SPI.h>
#define BME_SCK 18
#define BME_MISO 19
#define BME_MOSI 23
#define BME_CS 5
#define ANALOGUERAIN 34
#define DIGIGTALRAIN 36
#define BATTERVOLTPORT 35
#define R2 100 //voltage divider resistance (kOhm)
#define R1 220 //voltage divider resistance (kOhm)

#define SEALEVELPRESSURE_HPA (1013.25)

//Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

unsigned long delayTime;
float rain_analog; 
int rain_digital;
float bat_voltage;

void setup() {
  // bat_voltage = 0;
  Serial.begin(9600);
  Serial.println(F("BME280 test"));
  bool status;
  pinMode(DIGIGTALRAIN, INPUT);
  rain_digital = LOW;
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin();  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  Serial.println("-- Default Test --");
  delayTime = 1000;

  Serial.println();
}


void loop() { 
  read_Rain();
  read_BatVoltage();
  printValues();
  delay(delayTime);
}

void printValues() {
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  
  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");
  Serial.print("Analogue Rain = ");
  Serial.print(rain_analog);
  Serial.println(" V");
  Serial.print("Digital Rain= ");
  Serial.println(rain_digital);

  Serial.print("Battery Voltage = ");
  Serial.print(bat_voltage);
  Serial.println(" V");

  Serial.println();
}
void read_Rain(){
  rain_analog = float(analogRead(ANALOGUERAIN)) * (3.3/4096);
  rain_digital = digitalRead(DIGIGTALRAIN);
}

void read_BatVoltage(){
  bat_voltage = (analogRead(BATTERVOLTPORT))*(3.3/4096)*(float(R2+R1)/float(R1))  ;
}