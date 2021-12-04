#include <WiFi.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include "Credentials.h"
#include <ArduinoJson.h> 


#ifndef __CREDENTIALS_H
    #define AP_PASSWORD "Your accesspoint password  here"

    // Broker credentials (comment away if  you don't need a broker) Lower in this file
    #define BROKER_USER "your broker username"
    #define BROKER_PASSWORD "your broker password"
    #define BROKER_HOST IPAddress(192,168,1,0) // your broker ip address
    #define BROKER_PORT "yourbrokerport"
#endif


// Wifi and MQTT:
extern AsyncMqttClient mqttClient;
extern Ticker mqttReconnectTimer;
extern int WiFiConnectTimeout; //in ms

// WiFiEventHandler wifiConnectHandler;
// WiFiEventHandler wifiDisconnectHandler;
extern WiFiManager wifiManager;

//*-----MQTT Topics-----------*
#define TOPIC_STATE "weatherstation/state"
// #define TOPIC_ATTRIBUTES "weatherstation/attributes"
#define TOPIC_TEMP_CONFIG "homeassistant/sensor/weatherstation01/temperature/config" 
#define TOPIC_HUM_CONFIG "homeassistant/sensor/weatherstation01/humidity/config"
#define TOPIC_PRES_CONFIG "homeassistant/sensor/weatherstation01/pressure/config"
#define TOPIC_BATVOLT_CONFIG "homeassistant/sensor/weatherstation01/battery_voltage/config"
#define TOPIC_RAINVOLT_CONFIG "homeassistant/sensor/weatherstation01/rain_voltage/config"
#define TOPIC_RAINSENSOR_CONFIG "homeassistant/binary_sensor/weatherstation01/rain_voltage/config"

void onMqttConnect(bool sessionPresent);


void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
//
void onMqttSubscribe(uint16_t packetId, uint8_t qos);
void onMqttUnsubscribe(uint16_t packetId);

void onMqttPublish(uint16_t packetId) ;
bool publish_state(char *payload);
void publish_config();
void WiFiEvent(WiFiEvent_t event);
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
void connectToWiFi();