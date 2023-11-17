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
#define userInPumpState   "dripSystem/userInPumpState"


#define led  2  //Built in LED


// ---------- ENTER PIN USED HERE ---------------
// Water Level Sensor Pins
#define prox_far    23
#define prox_close  22

// Soil Moisture Pins
#define soilPin    32

// Relays Pins
#define pumpRelay   1
#define valveRelay  3

// ---------- TIMING VARIABLE ---------------
unsigned long currTime = 0;
unsigned long sendDelay = 1000;


float waterLevel = 0;
float moisture = 0;
int moistureThreshold = 60;
bool pumpState = 0;


void topicsSubscribe() {
  client.subscribe(humidityCalibrate);
  client.subscribe(userInPumpState);
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("MQTT message received on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    // Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  // Serial.println();
  // Serial.println(messageTemp);

  if (topic == humidityCalibrate) {
    moistureThreshold = (int) messaageTemp;
  }
  else if(topic == userInPumpState) {
    pumpState = (int) messageTemp;
  }
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
  pinMode(led, OUTPUT);
  pinMode(prox_far, INPUT);
  pinMode(prox_close, INPUT);
  pinMode(soilPin, INPUT);
  pinMode(pumpRelay, OUTPUT);
  pinMode(valveRelay, OUTPUT);

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

  // Set Both Pump and Valve to Off
  digitalWrite(valveRelay,0);
  digitalWrite(pumpRelay,0);
}

void loop() {
  if (digitalRead(prox_far) == 1) {   // Turn On Valve
    digitalWrite(valveRelay,1);
  }
  else if (digitalRead(prox_close)) { // Turn Off Valve
    digitalWrite(valveRelay,0);
  }

  if (moisture < moistureThreshold) { // Turn On Pump
    digitalWrite(pumpRelay,1);
    pumpState = 1;
  }
  else {                              // Turn Off Pump
    digitalWrite(pumpRelay,0);
    pumpState = 0;
  }

  waterLevel = !(digitalRead(prox_close)) & !(digitalRead(prox_far));

  if (millis() > currTime + sendDelay) {
    client.publish(humidityData, String(moisture).c_str(), true);
    client.publish(waterLevel, String(waterLevel).c_str(), true);
    client.publish(pumpState, String(pumpState).c_str(), true);
    currTime = millis();
    
    Serial.print("Moisture: "); Serial.println(moisture);
    Serial.print("Water Level:"); Serial.println(waterLevel);
    Serial.print("Pump State:"); Serial.println(pumpState);
  }

  client.loop();
}
