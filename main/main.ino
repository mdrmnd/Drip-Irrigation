#include <WiFi.h>
#include <PubSubClient.h>

// Variables for MQTT
const char* ssid = "Arycast";
const char* password =  "hellofromkiwi";
const char* mqttServer = "broker.mqtt-dashboard.com";
const int mqttPort = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

// Out Topic
#define humidityTopic1  "dripSystem/humidityTopic1" // Main to be used for determining pump state
#define humidityTopic2  "dripSystem/humidityTopic2"
#define waterTopic    "dripSystem/waterLevel"
#define pumpTopic     "dripSystem/pumpState"

// Incoming Topic
#define humidityCalibrate "dripSystem/calibrateHumidity"
#define userInPumpState   "dripSystem/userInPumpState"


#define led  2  //Built in LED


// ---------- ENTER PIN USED HERE ---------------
// Water Level Sensor Pins
#define prox_far    23
#define prox_close  22

// Soil Moisture Pins
#define soil1Pin    32
#define soil2Pin    33

// Relays Pins
#define pumpRelay   21
#define valveRelay  19

// ---------- TIMING VARIABLE ---------------
unsigned long currTime = 0;
unsigned long sendDelay = 10;

// ---------- SENSOR VARIABLE ---------------
int waterLevel = 0;
float moisture1 = 0, moisture2 = 0;
int moistureThreshold = 60;
bool pumpState = 0;
bool valveState = 0;

// ---------- FUNCTIONS ---------------

void blink_LED();
float read_moisture(float min, float max, uint8_t pinName);

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
    moistureThreshold = messageTemp.toInt();
  }
  else if(topic == userInPumpState) {
    pumpState = (bool) messageTemp;
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
      topicsSubscribe();
      Serial.println("connected");
      
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
  pinMode(prox_far, INPUT); pinMode(prox_close, INPUT);
  pinMode(soil1Pin, INPUT); pinMode(soil2Pin, INPUT);
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
  moisture1 = read_moisture(0.0, 100.0, soil1Pin); // Main to Be used for activating pump
  moisture2 = read_moisture(0.0, 100.0, soil2Pin);

  if (moisture1 < (float)moistureThreshold) digitalWrite(pumpRelay, 1);
  else digitalWrite(pumpRelay, 0);

  // Proximity Sensor Note : 
  // 0 => level < sensor threshold (LED ON)
  // 1 => level > sensor threshold (LED OFF)
  waterLevel = digitalRead(prox_close) or digitalRead(prox_far);
  
  // Valve Relay Normally Close => Value 1 to turn it ON
  digitalWrite(valveRelay, waterLevel);

  

  if (millis() > currTime + sendDelay) {
    client.publish(humidityTopic1, String(moisture1).c_str(), true);
    client.publish(humidityTopic2, String(moisture2).c_str(), true);
    client.publish(waterTopic, String(waterLevel).c_str(), true);
    client.publish(pumpTopic, String(pumpState).c_str(), true);
    currTime = millis();
    
    // Data Printing
    //Serial.println("================NEW DATA================");
    // Serial.print("Moisture: "); Serial.println(moisture);
    // Serial.print("Water Level:"); Serial.println(waterLevel);
    // Serial.print("Pump State:"); Serial.println(pumpState);
    // Serial.print("Valve State:"); Serial.println(valveState);
    // Serial.print(digitalRead(prox_close)); Serial.println(digitalRead(prox_far));
    // Serial.println(waterLevel);
  }

  client.loop(); 
}

float read_moisture(float min, float max, uint8_t pinName) {
  float val = analogRead(pinName);
  float result = (max - min)/(4096.0-0.0) * val + min;
  return result;
}

void blink_LED() {
  if (millis() - currTime > 500) digitalWrite(led, 1);
  else digitalWrite(led, 0);
}