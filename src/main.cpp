#include <Arduino.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <OSCMessage.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <config.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

//https://www.electroschematics.com/sonoff-basic-wi-fi-switch/
#define PIN_LED 13
#define PIN_RELAY 12
#define BUT_PIN 0

char server_ip[16] = "192.168.1.10";
char udp_port[6]  = "5582";

WiFiManager wm;
IPAddress outIp;
WiFiUDP Udp;
int udp_p;

unsigned long previousCheck = 0;
unsigned long interval = 3000;


bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void start_udp(){
  Serial.println("Starting UDP");
  Udp.begin(udp_p);
}

void setupFs(){
  // LittleFS.format();
  Serial.println("mounting FS...");
  if(LittleFS.begin()) {
    Serial.println("mounted file system");
    if(LittleFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = LittleFS.open("/config.json", "r");
      if(configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if(json.success()) {
          Serial.println("\nparsed json");

          strcpy(server_ip, json["server_ip"]);
          strcpy(udp_port, json["udp_port"]);
          udp_p = String(udp_port).toInt();

        }
        else Serial.println("failed to load json config");
      }
    }
  }

  else Serial.println("failed to mount FS");
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
  if(!digitalRead(BUT_PIN)) {
    Serial.println();
    OSCMessage msg("/ID");
    char out[20];
    sprintf(out, "Device-%08X",ESP.getChipId());
    Serial.println("Button Pressed!");
    Serial.println(String(ESP.getChipId()));
    msg.add(out);
    Udp.beginPacket(server_ip, udp_p);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    delay(500);
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

void processLED(OSCMessage &msg){
  bool val = msg.getInt(0);
  if(val){
    digitalWrite(PIN_LED, LOW);
    Serial.println("LED HIGH");
  }
  else{
    digitalWrite(PIN_LED, HIGH);
    Serial.println("LED LOW");
  }
}

void receiveMsg(){
  OSCMessage msg;
  int size = Udp.parsePacket();
  if (size > 0) {
    while (size--) msg.fill(Udp.read());
    if (!msg.hasError()) {
      msg.dispatch("/POWER", processPOWER);
      msg.dispatch("/LED", processLED);
    }
    else Serial.println("Error in msg");
  }
}

void start_wifi() {
  bool res;

  setupFs();

  WiFiManagerParameter custom_ip_server("ip", "IP server", server_ip, 40);
  WiFiManagerParameter custom_udp_port("port", "UDP port", udp_port, 6);

  wm.setSaveConfigCallback(saveConfigCallback);
  wm.addParameter(&custom_ip_server);
  wm.addParameter(&custom_udp_port);

  res = wm.autoConnect();
  if(!res)  Serial.println("Failed to connect");
  else {
    Serial.println("connected...yeey :)");
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    Serial.println();
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  strcpy(server_ip, custom_ip_server.getValue());
  strcpy(udp_port, custom_udp_port.getValue());

  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["server_ip"] = server_ip;
    json["udp_port"]  = udp_port;

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) Serial.println("failed to open config file for writing");

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    udp_p = String(udp_port).toInt();
    outIp.fromString(server_ip);
    shouldSaveConfig = false;
  }

}

void reconnect() {
  while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(BUT_PIN, INPUT_PULLUP);
  digitalWrite(PIN_LED, LOW);
  delay(3000);
  if(!digitalRead(BUT_PIN)) {
    wm.resetSettings();
    Serial.println("Resetando...");
  }
  digitalWrite(PIN_LED, HIGH);
  start_wifi();
  start_udp();
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
  check_wifi();
  receiveMsg();
  button_press();
  reconnect();
}
