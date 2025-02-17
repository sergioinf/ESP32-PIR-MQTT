#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "time.h"


const char* ssid = "";
const char* wifi_password = "";

const char* mqtt_server = "";
const char* mqtt_pwd = "";
const char* mqtt_usr = "";
const char* mqtt_topic = "";
const char* mqtt_control_topic = "";



// NTP Servers:
const char* ntpServer = "pool.ntp.org";  // Principal NTP server
const char* ntpBackup1 = "time.google.com"; // Secondary NTP server
const char* ntpBackup2 = "time.nist.gov";  // Secondary NTP server
const long  gmtOffset_sec = 0; // Timezone offset in seconds
const int   daylightOffset_sec = 0; // Daylight offset in seconds
 
IPAddress dns(8,8,8,8);

const int PIR_SENSOR_OUTPUT_PIN = 21;  /* PIR sensor O/P pin */
int warm_up;


WiFiClient espClient;
PubSubClient client(espClient);


void setup() {
  Serial.begin(115200); /* Define baud rate for serial communication */

  Serial.println("Setting up wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifi_password);

  Serial.println("Connecting");

  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(100);
  }

  Serial.println();
  Serial.println("Connected to WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  delay(2000);


  Serial.println("Configure hour");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(5000);
  Serial.println(printLocalTime());

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
      Serial.println("Can't get the time");
  }

  Serial.print("Actual hour: ");
  Serial.printf("%02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  delay(1000);

  Serial.println("Setting up MQTT");
  client.setServer(mqtt_server, 1883);
  reconnect();
  String message = "Connected to broker";
  client.publish(mqtt_control_topic, message.c_str());
  delay(2000);
  
  // Pin initialization for sensor reading
  Serial.println("Setting up sensor settings");
  pinMode(PIR_SENSOR_OUTPUT_PIN, INPUT);
  Serial.println("Waiting For Power On Warm Up");
  delay(5000); /* Power On Warm Up Delay */

}

String printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }

  char buffer[80];
  strftime(buffer, 80, "%Y-%m-%dT%H:%M:%S.000000Z", &timeinfo);
  return buffer;
}

void reconnect() {
    // Retry connection
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        if (client.connect("ESP32Client", mqtt_usr, mqtt_pwd)) {
            Serial.println("Connected.");
        } else {
            Serial.print("Error, rc=");
            Serial.print(client.state());
            Serial.println(" Retrying in 5 seconds...");
            delay(5000);
        }
    }
}


void loop() {
  if (!client.connected()) {
      reconnect();
  }

  StaticJsonDocument<200> doc;
  doc["sensorName"] = "pir";
  doc["startTime"] = printLocalTime();


  int sensor_output;
  sensor_output = digitalRead(PIR_SENSOR_OUTPUT_PIN);
  if( sensor_output == LOW ){  
    Serial.print("No object in sight\n\n");
    doc["detected"] = 0;
  }else{
    Serial.print("Object detected\n\n");
    doc["detected"] = 1;
  } 

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  client.publish(mqtt_topic, jsonBuffer);
  delay(1000);
}