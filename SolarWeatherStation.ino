
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "RemoteSettings.h"
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


unsigned long currentMillis = 1; 
unsigned long previousMillis = 0; 
const long interval_loop  = 5000;



//Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

unsigned long delayTime;
float rain_analog; 
int rain_digital;
// String rain_state; 
float bat_voltage;

float bmetemp;
float bmepres;
float bmehum; 
float bmealt; 

char statebuffer[300];


void setup() {
  // bat_voltage = 0;
  // rain_state = "OFF";
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
  delayTime = 5000;
  wifiManager.setConnectTimeout(180);
  wifiManager.setTimeout(180);
  wifiManager.setWiFiAutoReconnect(true);
  wifiManager.setDebugOutput(true);
  //    wifiManager.resetSettings();// uncomment to reset stored wifi settings
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(BROKER_HOST, BROKER_PORT);
  // If your broker requires authentication (username and password), set them below
  mqttClient.setCredentials(BROKER_USER, BROKER_PASSWORD);




  WiFi.onEvent(WiFiEvent);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
  WiFiEventId_t eventID = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.disconnected.reason);
  }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

  Serial.print("WiFi Event ID: ");
  Serial.println(eventID);  
  // WiFi.removeEvent(eventID);
  Serial.println("Connecting to WiFi");
  wifiManager.autoConnect("Weatherstation_AP",AP_PASSWORD);
}


void loop() { 

  currentMillis = millis();
  if(currentMillis - previousMillis >= interval_loop){ // don't use delay(). Messes with Tickers
  read_bme();
  read_Rain();
  read_BatVoltage();
  printValues();
  set_state_structure();
  if (WiFi.isConnected()) {
    publish_state();
  }
    previousMillis = currentMillis;
  }
}

void printValues() {
  Serial.print("Temperature = ");
  Serial.print(bmetemp);
  Serial.println(" *C");

  
  Serial.print("Pressure = ");
  Serial.print(bmepres / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bmealt);
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bmehum);
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
  // if(rain_digital == 0){
  //   rain_state = "ON";
  // }
  // else{
  //   rain_state = "OFF";
  // }
}

void read_bme(){
  bmetemp = bme.readTemperature();
  bmehum = bme.readHumidity();
  bmepres = bme.readPressure();
  bmealt = bme.readAltitude(SEALEVELPRESSURE_HPA);

}

void read_BatVoltage(){
  bat_voltage = (analogRead(BATTERVOLTPORT))*(3.3/4096)*(float(R2+R1)/float(R1))  ;
}

void set_state_structure(){
  StaticJsonDocument<300> stateJson; //Memory pool
  
  stateJson["temperature"] = String(bmetemp);
  
  
  stateJson["humidity"] = String(bmehum);
  
  stateJson["pressure"] = String(bmepres);
  

  stateJson["rain_voltage"] = String(rain_analog);
 
  stateJson["rain_digital"] = String(rain_digital);
  
  stateJson["battery_voltage"] = String(bat_voltage);
  
  stateJson["pressurealtitude"] = String(bmealt);

  memset(statebuffer,0,sizeof(statebuffer));
  serializeJson(stateJson,statebuffer);
  // delay(5000);
 
}


void publish_state(){
 
  uint16_t packetIdPub1 = mqttClient.publish(TOPIC_STATE, 1, false, statebuffer); 
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", TOPIC_STATE, packetIdPub1);
  Serial.printf("Message : %s\n",statebuffer);
}



