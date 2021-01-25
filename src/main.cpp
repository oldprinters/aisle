#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BH1750.h>
#include "OneLed.h"

BH1750 lightMeter(0x23);

// Update these with values suitable for your network.

const char* ssid = "ivanych";
const char* password = "stroykomitet";
const char* mqtt_server = "192.168.1.34";
const int BUILTIN_LED = 17; //управление включением света
const int IR_DATA = 34; //датчик движения
const int PIN_BUTTON = 16;

OneLed light(BUILTIN_LED);
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

bool ledStatus{0};
int buttonStatus{};

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Serial.print("Message arrived [");
  // Serial.print(topic);
  // Serial.print("] ");
  String str = {};
  String strTopic = topic;
  for (int i = 0; i < length; i++) {
    str += (char)payload[i];
  }
  Serial.print(strTopic);
  Serial.print(": ");
  Serial.println(str);

  if(strTopic == "aisle/maxLevel"){
    light.setMaxLevel(str.toInt());
  } else if(strTopic == "aisle/light"){
    if ((char)payload[0] == '1') {
      light.setStat(StatLed::ON);
    } else {
      light.setStat(StatLed::OFF);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish("outTopic", "hello world");
      client.subscribe("aisle/light");
      client.subscribe("aisle/maxLevel");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//*********************************************
void ir_interr(){ //IRAM_ATTR
	static unsigned long millis_prev;
	if (millis() - 100 > millis_prev)
    buttonStatus = 1;
	millis_prev = millis();
}
//*********************************************
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(IR_DATA, INPUT);  //Датчик движения
  pinMode(PIN_BUTTON, 16);  //кнопка
  Serial.begin(115200);
  Wire.begin();
  //........................
  attachInterrupt(digitalPinToInterrupt(16), ir_interr, RISING);
  //---------------------------
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 Advanced begin"));
  }
  else {
    Serial.println(F("Error initialising BH1750"));
  }
  //------------------
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if(buttonStatus){
    buttonStatus = !buttonStatus;
    light.trigger();
  }

  unsigned long now = millis();
  if (now - lastMsg > 2000) {

    if (lightMeter.measurementReady()) {
      float lux = lightMeter.readLightLevel();
      Serial.print("Light: ");
      Serial.print(lux);
      Serial.println(" lx");
      if(lux < 1.){
        light.setStat(StatLed::ON);
      } else if(lux > 5){
        light.setStat(StatLed::OFF);
      }

    }

    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    // Serial.print("Publish message: ");
    // Serial.println(msg);
    client.publish("outTopic", msg);
  }
  light.cycle();
}