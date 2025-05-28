
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "RemoteSettings.h"
#include <SPI.h>
#include <math.h>
#include <esp_sleep.h>

// #define BME_SCK 18
// #define BME_MISO 19
// #define BME_MOSI 23
// #define BME_CS 5
#define BME_SCL 19
#define BME_SDA 18
#define BME_VCC 23
#define ANALOGUERAIN 34
#define DIGIGTALRAIN 36
#define LIGHTPORT 35
#define RAINPOWER 32
#define BATTERVOLTPORT 35
#define AVG_WINDOW 30
#define R2 100 //voltage divider resistance (kOhm)
#define R1 220 //voltage divider resistance (kOhm)
#define SEALEVELPRESSURE_HPA (1013.25)


unsigned long currentMillis = 1; 
unsigned long previousMillis = 0; 
unsigned long previous_publish = 0;
const long interval_loop  = 1000;
const long interval_publish = 5000;

bool all_valid = false;

#define DEEPSLEEP
#define DEEPSLEEPDURATION 300000 // how many ms in deep sleep
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
  Measurement *light;
};


struct Measurement temperature;
struct Measurement pressure;
struct Measurement humidity;
struct Measurement altitude;
struct Measurement rain_voltage;
struct Measurement raining;
struct Measurement battery; 
struct Measurement light;
struct Readings measurements;

struct Readings *ptr_measurements = &measurements;
Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
// Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

unsigned long delayTime;
float rain_analog; 
float light_reading;
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
float light_avg;
// float rain_voltage_avg;
float altitude_avg;

char statebuffer[300];
float press_array[AVG_WINDOW] = {0};
int press_counter;
float temp_array[AVG_WINDOW] = {0};
int temp_counter;
float hum_array[AVG_WINDOW] = {0};
int hum_counter;
float alt_array[AVG_WINDOW] = {0};
int alt_counter;
float light_array[AVG_WINDOW] = {0};
int light_counter;
// float rain_vlt_array[AVG_WINDOW];


void setup() {

  measurements.pressure = &pressure;
  measurements.altitude = &altitude;
  measurements.battery = &battery;
  measurements.humidity = &humidity;
  measurements.pressure = &pressure;
  measurements.rain_voltage = &rain_voltage;
  measurements.raining = &raining;
  measurements.temperature = &temperature;
  measurements.light = &light;


  pressure.valid = false;
  humidity.valid = false;
  temperature.valid = false;
  
  strcpy(temperature.name,"temperature");
  strcpy(humidity.name,"humidity");
  strcpy(pressure.name,"pressure");
  strcpy(altitude.name,"pressurealtitude");
  strcpy(rain_voltage.name,"rain_voltage");
  strcpy(raining.name,"rain_digital");
  strcpy(battery.name,"battery_voltage");
  strcpy(light.name,"lightsensor_voltage");



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
  
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);

  mqttClient.setServer(BROKER_HOST, BROKER_PORT);
  // If your broker requires authentication (username and password), set them below
  mqttClient.setCredentials(BROKER_USER, BROKER_PASSWORD);
  WiFi.onEvent(WiFiEvent);
  // WiFi.onEvent(WiFiGotIP, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
  previous_publish = millis();
  #ifdef DEEPSLEEP
    esp_sleep_enable_timer_wakeup(DEEPSLEEPDURATION * 1000); // in microseconds
  #endif
  memset(press_array,NAN, sizeof(press_array));
  memset(temp_array,NAN, sizeof(temp_array));
  memset(hum_array,NAN, sizeof(hum_array));
  memset(alt_array,NAN, sizeof(alt_array));
  memset(light_array,NAN, sizeof(light_array));

  press_counter = 0;
  temp_counter = 0;
  hum_counter = 0;
  alt_counter = 0;
  light_counter = 0;
  previousMillis = millis();
 
}


void loop() { 
  currentMillis = millis();
  if(currentMillis - previousMillis >= interval_loop){ // don't use delay(). Messes with Tickers
    read_bme();    
    read_Light();   
    // printValues();
    
    previousMillis = currentMillis;
  }
  if((currentMillis - previous_publish >= interval_publish) && (hum_counter>=AVG_WINDOW && press_counter >=AVG_WINDOW && temp_counter >= AVG_WINDOW)){       

      if(WiFi.status() != WL_CONNECTED){
        connectToWiFi();
      }
      
      if (mqttClient.connected() && all_valid) {
         read_Rain();
         read_BatVoltage();
         set_state_structure();
         publish_state(statebuffer);        
      }

      else{
        Serial.println("Not all measurements are valid");
        Serial.printf("Pressure: %i, Temperature %i, Humidity: %i\n\n",pressure.valid,temperature.valid,humidity.valid);
      }
      previous_publish = currentMillis;
    }
    if(last_packagid == last_successfull_packagid){ //if successfully published
      #ifdef DEEPSLEEP
      Serial.printf("Going to deep sleep for %i seconds\n",DEEPSLEEPDURATION/1000);
      esp_deep_sleep_start();
      #endif
    }

    
}

void printValues() {

  Serial.printf("Temperature = %f*C, avg = %f*C\n",bmetemp,temperature_avg);
  Serial.printf("Humidity = %f%, avg = %f%\n",bmehum,humidity_avg);
  Serial.printf("Pressure = %fPa, avg = %fPa\n",bmepres,pressure_avg); 

  Serial.print("Approx. Altitude = ");
  Serial.print(bmealt);
  Serial.println(" m");

  Serial.printf("Rain Voltage = %fV\n",rain_analog);

  Serial.print("Digital Rain= ");
  Serial.println(rain_digital);

  Serial.print("Battery Voltage = ");
  Serial.print(bat_voltage);
  Serial.println(" V");
  Serial.print("\n\n");
}
void read_Rain(){

  digitalWrite(RAINPOWER,HIGH);
  delay(100);
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
  digitalWrite(RAINPOWER,LOW);
}
void read_Light(){
  light_reading = float(analogRead(LIGHTPORT)) * (3300.0/4096.0);
  light.valid = check_validity_light_voltage(light_reading);
  if (light.valid){
    appendAndShiftArray(light_array,light_reading,AVG_WINDOW,&light_counter); //update window
    light_avg = calculate_Average(light_array,AVG_WINDOW,light_counter);
    light.val = light_avg;
  }
  Serial.printf("Light Reading: %f\n", light_reading);
  Serial.printf("Light Reading average: %f\n", light_avg);

}

void read_bme(){ // Read sensors
  bmetemp = bme.readTemperature();
  bmehum = bme.readHumidity();
  bmepres = bme.readPressure();
  bmealt = bme.readAltitude(SEALEVELPRESSURE_HPA);


  if(check_validity_temperature(bmetemp)){    //check if newest reading is valid  
    appendAndShiftArray(temp_array,bmetemp,AVG_WINDOW,&temp_counter); //update window
    temperature_avg=calculate_Average(temp_array,AVG_WINDOW,temp_counter);
    temperature.val= temperature_avg;
    Serial.printf("Pressure counter: %i\n",press_counter);  
   
  }
   else{
      temperature.valid= false; 
    }
    temperature.valid=check_validity_temperature(temp_array[0]);
   
  if(check_validity_humidity(bmehum)){    
    appendAndShiftArray(hum_array,bmehum,AVG_WINDOW,&hum_counter);
    humidity_avg=calculate_Average(hum_array,AVG_WINDOW,hum_counter);
    humidity.val = humidity_avg;
  
    
  }
  else{
      humidity.valid = false; 
    }
  humidity.valid = check_validity_humidity(hum_array[0]);
  
  if(check_validity_pressure(bmepres)){
    appendAndShiftArray(press_array,bmepres,AVG_WINDOW,&press_counter); 
    pressure_avg=calculate_Average(press_array,AVG_WINDOW,press_counter);
      pressure.val = pressure_avg;
  }
  else{
    pressure.valid = false;
    }  
  pressure.valid = check_validity_pressure(press_array[0]); 

  if(pressure.valid && temperature.valid && humidity.valid){
    all_valid = true;
  }
  else{
    all_valid = false;
  }

}

void read_BatVoltage(){
  Serial.print("Reading batteryvoltage\n");
  delay(100);
  bat_voltage = (analogRead(BATTERVOLTPORT))*(3.3/4096)*(float(R2+R1)/float(R1))  ;

  battery.val = bat_voltage; 
  if(check_validity_battery_voltage(bat_voltage)){
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
  if(ptr_measurements->light->valid){
    stateJson[ptr_measurements->light->name] = ptr_measurements->light->val; 
  }
  else{
    stateJson[ptr_measurements->light->name] = NAN;
  }

  memset(statebuffer,0,sizeof(statebuffer));
  serializeJson(stateJson,statebuffer);
  // delay(5000);
 
}



void appendAndShiftArray(float arr[],float val, int length, int *counter){
  // Shift values to the left
  for(int i = 0; i < length-1; i++){
      arr[i]=arr[i+1];      
  }
  arr[length-1] = val; //Append value at the last entry 
  if(*counter<length){
    *counter+=1;
  }
}

float calculate_Average(float arr[], int length, int counter){
  float avg = 0;
  if(counter==0){ //if array is empty, would result in negative index in next loop
    return 0;
  }
  for(int i =(length-counter); i<length;i++){ // counter determines how much of the array is filled from the right. sum from there.
    avg += arr[i];
  }
  return avg/float(counter);
}

bool check_validity_pressure(float val){
  if(val>50000 && val <150000){
    return true;
  }
  else{
    return false;
  }
}

bool check_validity_humidity(float val){
  if(val>=0 && val <= 100){
    return true;
  }
  else{
    return false;
  }
}

bool check_validity_temperature(float val){
  if(val>= -30 && val<= 70){
    return true;
  }
  else{
    return false;
  }
}

bool check_validity_altitude(float val){
  if(val>-500 && val<= 15000){
    return true;
  }
  else{
    return false;
  }
}

bool check_validity_rain_voltage(float val){
  if(val>=0 && val<=4){
    return true;
  }
  else{
    return false;
  }
}


bool check_validity_battery_voltage(float val){
  if(val>=0 && val <= 10){
    return true;
  }
  else{
    return false;
  }
}

bool check_validity_light_voltage(float val){
  if(val>=0 && val <= 5000){
    return true;
  }
  else{
    return false;
  }
}