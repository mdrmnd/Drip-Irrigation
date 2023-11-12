#include <WiFi.h>
#include <PubSubClient.h>

// Variables for MQTT
const char* ssid = "ssid";
const char* password =  "pass";
const char* mqttServer = "broker.mqtt-dashboard.com";
const int mqttPort = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

// Out Topic
#define humidityData  "dripSystem/humidityData"
#define waterLevel    "dripSystem/waterLevel"
#define pumpState     "dripSystem/pumpState"

// Incoming Topic
#define humidityCalibrate "dripSystem/calibrateHumidity"


#define led  2  //Built in LED


// ---------- ENTER PIN USED HERE ---------------
// Water Level Sensor Pins
#define trig 
#define echo 

// Soil Moisture Pins
#define soilData

// Relays Pins
#define pumpRelay
#define valveRelay

void topicsSubscribe() {
  client.subscribe(humidityCalibrate);
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("MQTT message received on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  Serial.println(messageTemp);
}

void reconnect() {
  // Loop until we're reconnected
  int counter = 0;
  while (!client.connected()) {
    if (counter==5){
      ESP.restart();
    }
    counter+=1;
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
   
    if (client.connect("dripSystemWeb")) {
      Serial.println("connected");
      topicsSubscribe();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  topicsSubscribe();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  // begin Wifi connect
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //end Wifi connect

  client.setServer(mqttServer, 1883);
  client.setCallback(mqttCallback);

  delay(5000);

  if (!client.connected()){
    reconnect();
  }

  pinMode(led, OUTPUT);

}

void loop() {
  

  client.loop();
}
