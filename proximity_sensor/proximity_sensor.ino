int e18_sensor = 2;
void setup() {
pinMode (e18_sensor, INPUT);
Serial.begin(9600);
}

void loop() {
int objek = digitalRead(e18_sensor);
Serial.print("Status Objek: ");
Serial.println(objek);
delay(500);
}
