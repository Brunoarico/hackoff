#include <Arduino.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <OSCMessage.h>
#include <config.h>

//https://www.electroschematics.com/sonoff-basic-wi-fi-switch/
#define PIN_LED 13
#define PIN_RELAY 12
#define BUT_PIN 0

IPAddress outIp;
WiFiUDP Udp;

unsigned long previousCheck = 0;
unsigned long interval = 3000;
bool state = true;
bool state2 = true;

void start_udp(){
  Serial.println("Starting UDP");
  Udp.begin(udp_p);
}

void check_wifi() {
  unsigned long currentCheck = millis();
  if ((WiFi.status() != WL_CONNECTED) && (currentCheck - previousCheck>=interval)) {
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousCheck = currentCheck;
  }
}

void button_press () {
  OSCMessage msg("/ID");
  char out[20];
  if(digitalRead(BUT_PIN) == false && state == true) {
    Serial.println("on");
    sprintf(out, " 1Device-%08X",ESP.getChipId());
    Serial.println(String(ESP.getChipId()));
    msg.add(out);
    Udp.beginPacket(server_ip, udp_p);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    state = false;
    delay(300);
  }
  else if(digitalRead(BUT_PIN) == true && state == false) {

    Serial.println("off");
    sprintf(out, " 0Device-%08X",ESP.getChipId());
    Serial.println(String(ESP.getChipId()));
    msg.add(out);
    Udp.beginPacket(server_ip, udp_p);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    state = true;
    delay(300);
  }
}

void processPOWER(OSCMessage &msg){
  bool val = msg.getInt(0);
  if(val){
    digitalWrite(PIN_RELAY, HIGH);
    Serial.println("RELAY HIGH");
  }
  else{
    digitalWrite(PIN_RELAY, LOW);
    Serial.println("RELAY LOW");
  }
}

void interruptor_press () {
  OSCMessage msg("/Int");
  char out[20];
  if(digitalRead(BUT_PIN) == false && state2 == true) {
    Serial.println("interruptor on");
    sprintf(out, " 1Device-%08X",ESP.getChipId());
    Serial.println(String(ESP.getChipId()));
    msg.add(out);
    Udp.beginPacket(server_ip, udp_p);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    state2 = false;
    delay(300);
  }
  else if(digitalRead(BUT_PIN) == true && state2 == false) {
    Serial.println("interruptor off");
    sprintf(out, " 0Device-%08X",ESP.getChipId());
    Serial.println(String(ESP.getChipId()));
    msg.add(out);
    Udp.beginPacket(server_ip, udp_p);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    state2 = true;
    delay(300);
  }
}

void receiveMsg(){
  OSCMessage msg;
  int size = Udp.parsePacket();
  if (size > 0) {
    while (size--) msg.fill(Udp.read());
    if (!msg.hasError()) {
      msg.dispatch("/POWER", processPOWER);
    }
    else Serial.println("Error in msg");
  }
}


void start_wifi() {
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  //WiFi.setAutoReconnect(true);
  //WiFi.persistent(true);
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, INPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(BUT_PIN, INPUT_PULLUP);
  digitalWrite(PIN_LED, LOW);
  delay(1000);
  digitalWrite(PIN_LED, HIGH);
  start_wifi();
  start_udp();
}

void loop() {
  receiveMsg();
  button_press();
  interruptor_press();
}
