#include "RemoteSettings.h"

int WiFiConnectTimeout = 20000;


AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

uint16_t last_packagid = -1;
uint16_t last_successfull_packagid = -2;


void publish_config(){

  Serial.print("\nPublishing MQTT Discovery Configuration Payload\n\n");
    // TEMPERATURE
    StaticJsonDocument<300> tempdoc;
    tempdoc["dev_cla"] = "temperature"; 
    tempdoc["unit_of_meas"] = "°C";
    tempdoc["name"] = "weatherstation_01_temperature";
    tempdoc["stat_t"] = TOPIC_STATE;
    tempdoc["val_tpl"] = "{{ value_json.temperature }}";
    tempdoc["stat_cla"] = "measurement";

    char buffer[300];
    serializeJson(tempdoc,buffer);

    uint16_t packetIdPub0 = mqttClient.publish(TOPIC_TEMP_CONFIG,1 , true, buffer);
    last_packagid = packetIdPub0;
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i \n", TOPIC_TEMP_CONFIG, packetIdPub0);
    Serial.printf("Message : %s\n",buffer);


    memset(buffer,0, sizeof(buffer));

    // Humidity
    StaticJsonDocument<300> humdoc;
    humdoc["dev_cla"] = "humidity"; 
    humdoc["unit_of_meas"] = "%";
    humdoc["name"] = "weatherstation_01_humidity";
    humdoc["stat_t"] = TOPIC_STATE;
    humdoc["val_tpl"] = "{{ value_json.humidity }}";
    humdoc["stat_cla"] = "measurement";

    serializeJson(humdoc,buffer);
  
    uint16_t packetIdPub2 = mqttClient.publish(TOPIC_HUM_CONFIG,1 , true, buffer);
    last_packagid = packetIdPub2;
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i \n", TOPIC_HUM_CONFIG, packetIdPub2);
    Serial.printf("Message : %s\n",buffer);

    memset(buffer,0, sizeof(buffer));

    // Pressure
    StaticJsonDocument<300> presdoc;
    presdoc["dev_cla"] = "pressure"; 
    presdoc["unit_of_meas"] = "Pa";
    presdoc["name"] = "weatherstation_01_pressure";
    presdoc["stat_t"] = TOPIC_STATE;
    presdoc["val_tpl"] = "{{ value_json.pressure }}";
    presdoc["stat_cla"] = "measurement";

    serializeJson(presdoc,buffer);
   
    uint16_t packetIdPub3 = mqttClient.publish(TOPIC_PRES_CONFIG,1 , true, buffer);
    last_packagid = packetIdPub3;
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i \n", TOPIC_PRES_CONFIG, packetIdPub3);
    Serial.printf("Message : %s\n",buffer);

    memset(buffer,0, sizeof(buffer));

    //batteryvoltage
    StaticJsonDocument<300> batvdoc;
    batvdoc["dev_cla"] = "voltage"; 
    batvdoc["unit_of_meas"] = "V";
    batvdoc["name"] = "weatherstation_01_battery_voltage";
    batvdoc["stat_t"] = TOPIC_STATE;
    batvdoc["val_tpl"] = "{{ value_json.battery_voltage }}";
    batvdoc["stat_cla"] = "measurement";

    serializeJson(batvdoc,buffer);
   
    uint16_t packetIdPub4 = mqttClient.publish(TOPIC_BATVOLT_CONFIG,1 , true, buffer);
    last_packagid = packetIdPub4;
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i \n", TOPIC_BATVOLT_CONFIG, packetIdPub4);
    Serial.printf("Message : %s\n",buffer);

    memset(buffer,0, sizeof(buffer));

    //rain voltage
    StaticJsonDocument<300> rainvdoc;
    rainvdoc["dev_cla"] = "voltage"; 
    rainvdoc["unit_of_meas"] = "V";
    rainvdoc["name"] = "weatherstation_01_rainsensor_voltage";
    rainvdoc["stat_t"] = TOPIC_STATE;
    rainvdoc["val_tpl"] = "{{ value_json.rain_voltage }}";
    rainvdoc["stat_cla"] = "measurement";

    serializeJson(rainvdoc,buffer);

    uint16_t packetIdPub5 = mqttClient.publish(TOPIC_RAINVOLT_CONFIG,1 , true, buffer);
    last_packagid = packetIdPub5; 
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i \n", TOPIC_RAINVOLT_CONFIG, packetIdPub5);
    Serial.printf("Message : %s\n",buffer);

    memset(buffer,0, sizeof(buffer));

    //rain sensor
    StaticJsonDocument<300> rainsensordoc;

    rainsensordoc["name"] = "weatherstation_01_rainsensor";
    rainsensordoc["stat_t"] = TOPIC_STATE;
    rainsensordoc["dev_cla"] = "moisture"; 
    rainsensordoc["pl_on"] = "0";
    rainsensordoc["pl_off"] = "1"; 
    rainsensordoc["val_tpl"] = "{{ value_json.rain_digital }}";

    serializeJson(rainsensordoc,buffer);
   
    uint16_t packetIdPub6 = mqttClient.publish(TOPIC_RAINSENSOR_CONFIG,1 , true, buffer);
    last_packagid = packetIdPub6;
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i \n", TOPIC_RAINSENSOR_CONFIG, packetIdPub6);
    Serial.printf("Message : %s\n",buffer);

    memset(buffer,0, sizeof(buffer));

    // Light sensor
    JsonDocument lightsensordoc;
    lightsensordoc["name"] = "weatherstation_01_lightsensor";
    lightsensordoc["stat_t"] = TOPIC_STATE;
    lightsensordoc["dev_cla"] = "voltage";
    lightsensordoc["unit_of_meas"] = "mV";
    lightsensordoc["val_tpl"] = "{{ value_json.lightsensor_voltage }}";
    lightsensordoc["stat_cla"] = "measurement";

    serializeJson(lightsensordoc,buffer);

    uint16_t packetIdPub7 = mqttClient.publish(TOPIC_LIGHTSENSOR_CONFIG,1, true, buffer);
    last_packagid = packetIdPub7;
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i \n", TOPIC_LIGHTSENSOR_CONFIG, packetIdPub7);
    Serial.printf("Message : %s\n",buffer);
    memset(buffer,0, sizeof(buffer));

    Serial.print("\n\n");
}




void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  publish_config();
}


void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}
//
void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  last_successfull_packagid = packetId;
}


 void publish_state(char *payload){

 Serial.print("\n Publishing State payload\n\n");
  uint16_t packetIdPub1 = mqttClient.publish(TOPIC_STATE, 1, false, payload); 
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i \n", TOPIC_STATE, packetIdPub1);
  last_packagid = packetIdPub1;
  Serial.printf("Message : %s\n\n",payload);
}

//void onMqttSubscribe(uint16_t packetId) {
//  if(LED_ON){
//  digitalWrite(LEDPINWIFI,HIGH);
//  }
//  Serial.print("Subscribe acknowledged.");
//  Serial.print("  packetId: ");
//  Serial.println(packetId);
//}


void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event) {
    case SYSTEM_EVENT_WIFI_READY: 
      Serial.println("WiFi interface ready");
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      Serial.println("Completed scan for access points");
      break;
    case SYSTEM_EVENT_STA_START:
      Serial.println("WiFi client started");
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println("WiFi clients stopped");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("Connected to access point");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("Disconnected from WiFi access point");
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      Serial.println("Authentication mode of access point has changed");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println("Lost IP address and IP address is reset to 0");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      break;
    case SYSTEM_EVENT_AP_START:
      Serial.println("WiFi access point started");
      break;
    case SYSTEM_EVENT_AP_STOP:
      Serial.println("WiFi access point  stopped");
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      Serial.println("Client connected");
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Serial.println("Client disconnected");
      break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      Serial.println("Assigned IP address to client");
      break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
      Serial.println("Received probe request");
      break;
    case SYSTEM_EVENT_GOT_IP6:
      Serial.println("IPv6 is preferred");
      break;
    case SYSTEM_EVENT_ETH_START:
      Serial.println("Ethernet started");
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("Ethernet stopped");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet connected");
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet disconnected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.println("Obtained IP address");
      break;
    default: break;
}}


void connectToWiFi(){
  int time = millis();
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFISSID,WIFIPASS);
  while(WiFi.status() != WL_CONNECTED && (millis()-time) < WiFiConnectTimeout){
    Serial.print("."); 
    delay(1000);
  }
  if((millis()-time)>WiFiConnectTimeout){
    Serial.print("Connection failed\n");
  }
}

// void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
//   Serial.println("WiFi connected");
//   Serial.println("IP address: ");
//   Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
//   connectToMqtt();
// }
