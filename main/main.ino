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
#define waterTopic      "dripSystem/waterLevel"
#define valveTopic      "dripSystem/valveState"
#define pumpTopic       "dripSystem/pumpState"

// Incoming Topic
const char* humidityCalibrate    = "dripSystem/calibrateHumidity";
const char* ON_userPumpState     = "dripSystem/ON_userPumpState";
const char* OFF_userPumpState    = "dripSystem/OFF_userPumpState";
const char* ON_userValveState    = "dripSystem/ON_userValveState";
const char* OFF_userValveState   = "dripSystem/OFF_userValveState";


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
unsigned long sendDelay = 1000;
unsigned long ledTime = 0;

// ---------- SENSOR VARIABLE ---------------
int waterLevel = 0;
float moisture1 = 0, moisture2 = 0;
int moistureThreshold = 60;
bool pumpState = 0; int UserPumpState = 0;
bool valveState = 0; int UserValveState = 0;

// ---------- FUNCTIONS ---------------

void blink_LED();
float read_moisture(float min, float max, uint8_t pinName);

void topicsSubscribe() {
  client.subscribe(humidityCalibrate);
  client.subscribe(ON_userPumpState);
  client.subscribe(OFF_userPumpState);
  client.subscribe(ON_userValveState);
  client.subscribe(OFF_userValveState);
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
  
  //Serial.println();

  if (String(topic) == String(humidityCalibrate)) {
    moistureThreshold = messageTemp.toInt();
  }
  else if((String(topic) == String(ON_userPumpState)) || (String(topic) == String(OFF_userPumpState))) {
    UserPumpState = messageTemp.toInt();
  }
  else if((String(topic) == String(ON_userValveState)) || (String(topic) == String(OFF_userValveState))) {
    UserValveState = messageTemp.toInt();
  }
  // Serial.print(messageTemp);
  // Serial.println();
  // Serial.print("User Input User Pump State: "); Serial.println(UserPumpState);
  // Serial.print("User Input User Valve State: "); Serial.println(UserValveState);
  // Serial.println();
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
  digitalWrite(led, 1);
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
    digitalWrite(led, 1);
    delay(250);
    digitalWrite(led, 0);
    delay(250);
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
  // Testing Purposes
  // moisture1 = 0; moisture2 = 0; waterLevel = 0;

  // Moisture Sensor
  moisture1 = read_moisture(0.0, 100.0, soil1Pin); // Main to Be used for activating pump
  moisture2 = read_moisture(0.0, 100.0, soil2Pin);

  // =============== PUMP ===============
  // Condition for Pump 
  if (UserPumpState == 2) {
    if (moisture1 >= (float)moistureThreshold) pumpState = 0;
    else pumpState = 1;
  }
  else if (UserPumpState == 1){
    if (millis() > currTime + 30000) pumpState = 0;
    else pumpState = 1;
  }
  else pumpState = 0;
  // Output Pump Relay (Normally Close)
  // 0 => Pump OFF
  // 1 => Pump ON
  digitalWrite(pumpRelay, pumpState);


  // Proximity Sensor Note : 
  // 0 => level < sensor threshold (LED ON)
  // 1 => level > sensor threshold (LED OFF)
  waterLevel = digitalRead(prox_close) or digitalRead(prox_far);

  // =============== Valve ===============
  // Condition for Valve
  if (UserValveState == 2) valveState = waterLevel;
  else if (UserValveState == 1){
    if (millis() > currTime + 30000) valveState = 0;
    else valveState = 1;
  }
  else valveState = 0;
  // Output Pump Relay (Normally Close)
  // 0 => Valve OFF
  // 1 => Vlave ON
  digitalWrite(valveRelay, valveState);

  

  if (millis() > currTime + sendDelay) {
    client.publish(humidityTopic1, String(moisture1).c_str(), true);
    client.publish(humidityTopic2, String(moisture2).c_str(), true);
    client.publish(waterTopic, String(waterLevel).c_str(), true);
    client.publish(valveTopic, String(valveState).c_str(), true);
    client.publish(pumpTopic, String(pumpState).c_str(), true);
    currTime = millis();
    
    // Data Printing
    // Serial.println("================NEW DATA================");
    // Serial.print("Moisture Threshold: "); Serial.println(moistureThreshold);
    // Serial.print("Moisture1: "); Serial.print(moisture1); Serial.print("\tMoisture2: "); Serial.println(moisture2);
    // Serial.print("Pump State:"); Serial.println(pumpState);
    // Serial.print("Water Level:"); Serial.println(waterLevel);
    // Serial.print("Valve State:"); Serial.println(valveState);
    //Serial.print(digitalRead(prox_close)); Serial.println(digitalRead(prox_far));
  }

  client.loop(); 
}

float read_moisture(float min, float max, uint8_t pinName) {
  float val = analogRead(pinName);
  val = 4095 - val;
  float result = (max - min)/(4096.0-0.0) * val + min;
  return result;
}