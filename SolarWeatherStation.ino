
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "RemoteSettings.h"
#include <SPI.h>
#include <math.h>
#include <esp_sleep.h>
// #define USEWIFIMANAGER
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
#define AVG_WINDOW 60
#define R2 100 //voltage divider resistance (kOhm)
#define R1 220 //voltage divider resistance (kOhm)
#define SEALEVELPRESSURE_HPA (1013.25)
#define DEEPSLEEPDURATION 30000 // how many ms in deep sleep

unsigned long currentMillis = 1; 
unsigned long previousMillis = 0; 
unsigned long previous_publish = 0;
const long interval_loop  = 1000;
const long interval_publish = 5000;

bool all_valid = false;



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

float pressure_avg;
float humidity_avg;
float temperature_avg;
float rain_voltage_avg;
float altitude_avg;

char statebuffer[300];
float press_array[AVG_WINDOW] = {0};
float temp_array[AVG_WINDOW] = {0};
float hum_array[AVG_WINDOW] = {0};
// float rain_vlt_array[AVG_WINDOW];
float alt_array[AVG_WINDOW] = {0};
bool success_publish = false;



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
  Serial.begin(9600);
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
  
  delayTime = 5000;
  
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
  #ifdef USEWIFIMANAGER
  wifiManager.setConnectTimeout(180);
  wifiManager.setTimeout(180);
  wifiManager.setWiFiAutoReconnect(true);
  wifiManager.setDebugOutput(true);
  wifiManager.autoConnect("Weatherstation_AP",AP_PASSWORD);
  #else
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFISSID,WIFIPASS);
  #endif
  previous_publish = millis();
  esp_sleep_enable_timer_wakeup(DEEPSLEEPDURATION * 1000); // in microseconds
}


void loop() { 
  success_publish = false;
  currentMillis = millis();
  if(currentMillis - previousMillis >= interval_loop){ // don't use delay(). Messes with Tickers
    read_bme();
    read_Rain();
    read_BatVoltage();    
    find_Averages();
    printValues();
    previousMillis = currentMillis;
  }
  if(currentMillis - previous_publish >= interval_publish){
      set_state_structure();
      if (WiFi.isConnected() && all_valid) {
        success_publish = publish_state();
        
      }
      else{
        Serial.println("Not all measurements are valid");
        Serial.printf("Pressure: %i, Temperature %i, Humidity: %i\n\n",pressure.valid,temperature.valid,humidity.valid);
      }
      previous_publish = currentMillis;
    }
    if(success_publish){
      Serial.println("Going to deep sleep");
      esp_deep_sleep_start();
    }

    
}

void printValues() {

  Serial.printf("Temperature = %f*C, avg = %f*C\n",bmetemp,temperature_avg);
  Serial.printf("Humidity = %f%, avg = %f%\n",bmehum,humidity_avg);
  Serial.printf("Pressure = %fPa, avg = %fPa\n",bmepres,pressure_avg); 

  Serial.print("Approx. Altitude = ");
  Serial.print(bmealt);
  Serial.println(" m");

 Serial.printf("Rain Voltage = %fV, avg = %fV\n",rain_analog,rain_voltage_avg);

  Serial.print("Digital Rain= ");
  Serial.println(rain_digital);

  Serial.print("Battery Voltage = ");
  Serial.print(bat_voltage);
  Serial.println(" V");
  Serial.print("\n\n");
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


  if(bmetemp >= -60 && bmetemp <= 100){    //check if newest reading is valid  
    appendAndShiftArray(temp_array,bmetemp,AVG_WINDOW); //update window
    if(temperature_avg >= -60 && temperature_avg <= 100){ //check if average reading is valid
        temperature.val= temperature_avg;
        temperature.valid = true; 
    }    
    else{
      temperature.valid= false; 
    }
    if(temp_array[0] == 0){ // if window hasn't been filled yet
      temperature.valid = false;
    }
  }
   
  if(bmehum >= 0 && bmehum <= 100){    
    appendAndShiftArray(hum_array,bmehum,AVG_WINDOW);
    if(humidity_avg >= 0 && humidity_avg <= 100){
      humidity.val = humidity_avg;
      humidity.valid = true;
    }
    else{
      humidity.valid = false; 
    }
    if(hum_array[0] == 0){
      humidity.valid = false;
  }
  }
  
  if(bmepres >= 10000 && bmepres <= 1e6){
    appendAndShiftArray(press_array,bmepres,AVG_WINDOW);
  if(pressure_avg >= 10000 && pressure_avg <= 1e6){
       pressure.valid = true;
       pressure.val = pressure_avg;
    }
  else{
    pressure.valid = false;
    }  
    if(press_array[0]==0){
      pressure.valid = false; 
    }   
  }  

  if(pressure.valid && temperature.valid && humidity.valid){
    all_valid = true;
  }
  else{
    all_valid = false;
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
  else{
    stateJson[ptr_measurements->temperature->name] = NAN;
  }
  if(ptr_measurements->humidity->valid){
  stateJson[ptr_measurements->humidity->name] =     ptr_measurements->humidity->val;
  }
  else{
    stateJson[ptr_measurements->humidity->name] = NAN;
  }
  if(ptr_measurements->pressure->valid){
  stateJson[ptr_measurements->pressure->name] =     ptr_measurements->pressure->val ;
  }
  else{
    stateJson[ptr_measurements->pressure->name] = NAN;
  }
  if(ptr_measurements->rain_voltage->valid){
  stateJson[ptr_measurements->rain_voltage->name] = ptr_measurements->rain_voltage->val;
  }
  else{
    stateJson[ptr_measurements->rain_voltage->name] = NAN;
  }
  if(ptr_measurements->raining->valid){
    
  stateJson[ptr_measurements->raining->name] = ptr_measurements->raining->val; 
  }
  else{
    stateJson[ptr_measurements->raining->name] = NAN;
  }
  if(ptr_measurements->battery->valid){
    stateJson[ptr_measurements->battery->name] = ptr_measurements->battery->val; 
  }
  else{
    stateJson[ptr_measurements->battery->name] = NAN;
  }

  memset(statebuffer,0,sizeof(statebuffer));
  serializeJson(stateJson,statebuffer);
  // delay(5000);
 
}


bool publish_state(){
 
  uint16_t packetIdPub1 = mqttClient.publish(TOPIC_STATE, 1, false, statebuffer); 
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", TOPIC_STATE, packetIdPub1);
  Serial.printf("Message : %s\n",statebuffer);
  return true;
}

void appendAndShiftArray(float arr[],float val, int length){
  // Shift values to the left
  for(int i = 0; i < length-1; i++){
      arr[i]=arr[i+1];      
  }
  arr[length-1] = val; //Append value at the last entry 
}

float calculate_Average(float arr[], int length){
  float avg = 0;
  for(int i =0; i<length;i++){
    avg += arr[i];
  }
  return avg/float(length);
}

void find_Averages(){
  pressure_avg = calculate_Average(press_array,AVG_WINDOW);
  humidity_avg = calculate_Average(hum_array,AVG_WINDOW);
  temperature_avg = calculate_Average(temp_array,AVG_WINDOW);
  // rain_voltage_avg = calculate_Average(rain_vlt_array,AVG_WINDOW);
}