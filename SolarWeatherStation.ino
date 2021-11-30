
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "RemoteSettings.h"
#include <SPI.h>

// #define BME_SCK 18
// #define BME_MISO 19
// #define BME_MOSI 23
// #define BME_CS 5
#define BME_SCL 19
#define BME_SDA 18
#define BME_VCC 23
#define ANALOGUERAIN 34
#define DIGIGTALRAIN 36
#define RAINPOWER 32
#define BATTERVOLTPORT 35
#define R2 100 //voltage divider resistance (kOhm)
#define R1 220 //voltage divider resistance (kOhm)
#define SEALEVELPRESSURE_HPA (1013.25)


unsigned long currentMillis = 1; 
unsigned long previousMillis = 0; 
const long interval_loop  = 5000;

typedef struct Measurement{
  char name[50];
  float val; 
  bool valid;  
};

typedef struct Readings{
  Measurement *humidity;
  Measurement *pressure; 
  Measurement *temperature;
  Measurement *rain_voltage;
  Measurement *raining;
  Measurement *altitude; 
  Measurement *battery; 
};


struct Measurement temperature;
struct Measurement pressure;
struct Measurement humidity;
struct Measurement altitude;
struct Measurement rain_voltage;
struct Measurement raining;
struct Measurement battery; 
struct Readings measurements;

struct Readings *ptr_measurements = &measurements;
Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
// Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

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
  measurements.pressure = &pressure;
  measurements.altitude = &altitude;
  measurements.battery = &battery;
  measurements.humidity = &humidity;
  measurements.pressure = &pressure;
  measurements.rain_voltage = &rain_voltage;
  measurements.raining = &raining;
  measurements.temperature = &temperature;

  
  strcpy(temperature.name,"temperature");
  strcpy(humidity.name,"humidity");
  strcpy(pressure.name,"pressure");
  strcpy(altitude.name,"pressurealtitude");
  strcpy(rain_voltage.name,"rain_voltage");
  strcpy(raining.name,"rain_digital");
  strcpy(battery.name,"battery_voltage");


  // bat_voltage = 0;
  // rain_state = "OFF";
  Serial.begin(19200);
  Serial.println(F("BME280 test"));
  bool status;
  pinMode(DIGIGTALRAIN, INPUT);
  pinMode(RAINPOWER, OUTPUT);
  rain_digital = LOW;
  pinMode(BME_VCC, OUTPUT);
  digitalWrite(BME_VCC,HIGH);
  digitalWrite(RAINPOWER,HIGH);
  delay(1000);
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  // status = bme.begin(); 
  status = bme.begin(0x76); 
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

  Serial.println("structure readings:");
  Serial.printf("pressure: %f\n",ptr_measurements->pressure->val);
}
void read_Rain(){
  rain_analog = float(analogRead(ANALOGUERAIN)) * (3.3/4096);
  rain_digital = digitalRead(DIGIGTALRAIN);

  rain_voltage.val = rain_analog;
  if(rain_analog >= 0 && rain_analog < 4){
    rain_voltage.valid = true;
  }
  else{
    rain_voltage.valid = false;
  }
  raining.val = rain_digital;
  if(rain_digital == HIGH || rain_digital == LOW){
    raining.valid = true;
  }
  else{
    raining.valid = false;
  }
}

void read_bme(){ // Read sensors
  bmetemp = bme.readTemperature();
  bmehum = bme.readHumidity();
  bmepres = bme.readPressure();
  bmealt = bme.readAltitude(SEALEVELPRESSURE_HPA);

  temperature.val= bmetemp;
  if(bmetemp >= -60 && bmetemp <= 100){    
    temperature.valid = true; 
  }
  else{
    temperature.valid= false; 
  }

  humidity.val = bmehum;
  if(bmehum >= 0 && bmehum <= 100){
    humidity.valid = true;
  }
  else{
    humidity.valid = false; 
  }

  pressure.val = bmepres;
  if(bmepres >= 1000 && bmepres <= 1e6){
    pressure.valid = true;
  }
  else{
    pressure.valid = false;
  }

}

void read_BatVoltage(){
  bat_voltage = (analogRead(BATTERVOLTPORT))*(3.3/4096)*(float(R2+R1)/float(R1))  ;

  battery.val = bat_voltage; 
  if(bat_voltage > 0 && bat_voltage < 10){
    battery.valid = true;
  }
  else{
    battery.valid = false;
  }
}

// Create JSON payload for MQTT
void set_state_structure(){
  StaticJsonDocument<300> stateJson; //Memory pool

  // Check for each measurements if it's valid
  if(ptr_measurements->temperature->valid){  
  stateJson[ptr_measurements->temperature->name] =  ptr_measurements->temperature->val;
  }
  if(ptr_measurements->humidity->valid){
  stateJson[ptr_measurements->humidity->name] =     ptr_measurements->humidity->val;
  }
  if(ptr_measurements->pressure->valid){
  stateJson[ptr_measurements->pressure->name] =     ptr_measurements->pressure->val ;
  }
  if(ptr_measurements->rain_voltage->valid){
  stateJson[ptr_measurements->rain_voltage->name] = ptr_measurements->rain_voltage->val;
  }
  if(ptr_measurements->raining->valid){
    
  stateJson[ptr_measurements->raining->name] =      ptr_measurements->raining->val; 
  }
  if(ptr_measurements->battery->valid){
  stateJson[ptr_measurements->battery->name] =      ptr_measurements->battery->val; 
  }

  memset(statebuffer,0,sizeof(statebuffer));
  serializeJson(stateJson,statebuffer);
  // delay(5000);
 
}


void publish_state(){
 
  uint16_t packetIdPub1 = mqttClient.publish(TOPIC_STATE, 1, false, statebuffer); 
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", TOPIC_STATE, packetIdPub1);
  Serial.printf("Message : %s\n",statebuffer);
}



