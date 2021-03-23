#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BH1750.h>
#include "OneLed.h"
#include "button.h"
#include "LedBlink.h"

BH1750 lightMeter(0x23);

// Update these with values suitable for your network.

const char* ssid = "ivanych";
const char* password = "stroykomitet";
const char* mqtt_server = "192.168.1.34";
const int BUILTIN_LED = 17; //управление включением света
const int IR_DATA = 34; //датчик движения
const int PIN_BUTTON = 16;
const int LED_BLINK_R = 26;  //мигающий светодиод постановки на охрану
const int LED_BLINK_G = 25;  //мигающий светодиод
const int LED_BLINK_B = 27;  //мигающий светодиод
//i2c sda-21, scl-22
OneLed light(BUILTIN_LED);
WiFiClient espClient;
PubSubClient client(espClient);
Timer tMotion(20000);
LedBlink ledBlinkR(LED_BLINK_R); //мигающий светодиод 
LedBlink ledBlinkG(LED_BLINK_G); //мигающий светодиод 
LedBlink ledBlinkB(LED_BLINK_B); //мигающий светодиод 

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value{};


const char* LedBlink_G="aisle_/ledBlinkG";
const char* LedBlink_B="aisle_/ledBlinkB";
const char* apb="aisle_/press_button";
const char* msg_motion="aisle_/motion";
const char* extLight = "aisle_/ext_light";
const char* topic_security = "aisle_/security";
const char* TopicMaxLevel = "aisle_/maxLevel";
const char* Topic_Light = "aisle_/light";
volatile int buttonStatus{};
volatile bool ir_motion{};
bool ledStatus{};
bool irLightOn{}; //свет включен по таймеру
bool lightStat{}; //состояние внешнего света
bool hardOn{};  //принудительное включение света
float lux{};  //яркость света в помещении
bool security{};  //стоит на охране

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

/*
aisle/maxLevel - максимальная яркость
aisle/light - включение / выключение
aisle/motion - обнаружино движение
aisle/ext_light - включен свет
aisle/press_button - была нажата кнопка
*/
void callback(char* topic, byte* payload, unsigned int length) {
  String str = {};
  String strTopic = topic;
  for (int i = 0; i < length; i++) {
    str += (char)payload[i];
  }
  // Serial.print(strTopic);
  // Serial.print(": ");
  // Serial.println(str);

  if(strTopic == TopicMaxLevel){
    light.setMaxLevel(str.toInt());
  } else if(strTopic == Topic_Light){
    if ((char)payload[0] == '1') {
      hardOn = true;
      light.setStat(StatLed::ON);
    } else {
      light.setStat(StatLed::OFF);
      hardOn = false;
    }
  } else if(strTopic == LedBlink_G)
      ledBlinkG.setStat(payload[0] - '0');
    else if(strTopic == LedBlink_B)
      ledBlinkB.setStat(payload[0] - '0');
}
//******************************************************
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
//      client.publish("outTopic", "hello world");
      client.subscribe("aisle_/light");
      client.subscribe("aisle_/maxLevel");
      client.subscribe("aisle_/ledBlinkB");
      client.subscribe("aisle_/ledBlinkG");
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
void button_interr(){ //IRAM_ATTR
	static unsigned long millis_prev;
	if (millis() - 100 > millis_prev)
    buttonStatus = 1;
	millis_prev = millis();
//  Serial.println("interrupt");
}
//*********************************************
void ir_interr(){
  ir_motion = true;
//  Serial.println("ir_interr");
}
//*********************************************
void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(IR_DATA, INPUT);  //Датчик движения
  pinMode(PIN_BUTTON, INPUT);  //кнопка
  Serial.begin(115200);
  Wire.begin();
  //........................
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), button_interr, RISING);
  attachInterrupt(digitalPinToInterrupt(IR_DATA), ir_interr, RISING);
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
//************************************
 void ir_motion_func(){
 if(ir_motion){
    ir_motion = false;
    if(!irLightOn) {
      client.publish(msg_motion, "1");
      client.publish(msg_motion, "0");
      irLightOn = true;
    }
    tMotion.setTimer();
    if(!hardOn){
        if(lux <= 2.){
          light.setStat(StatLed::ON);
        } else {
          light.setStat(StatLed::OFF);
        }
    }
  }
  if(irLightOn && tMotion.getTimer()){
      client.publish(msg_motion, "0");
      irLightOn = false;
    if(!hardOn){
      light.setStat(StatLed::OFF);
    }    
  }
}
//************************************  добавить режим folow me. Задержка 5 сек при выключении света
void fShort(){
  if(security){
    security = false;
    ledBlinkR.setStat(security);
    client.publish(topic_security, "0");
  } else {
    hardOn = !hardOn;
    light.setStat(hardOn? StatLed::ON : StatLed::OFF);
    const char* msg = hardOn? "1":"0";
    client.publish(apb, msg);
  }
}
//************************************* 
void fDouble(){
  security = !security;
  if(security){
    hardOn = false;
    light.setStat(StatLed::OFF);
  }
  ledBlinkR.setStat(security);
  client.publish(topic_security, (security? "1": "0"));
}
//************************************
void fLong(){
  if(security){
    security = false;
    ledBlinkR.setStat(security);
    client.publish(topic_security, "0");
  } else {
    light.setMaxLevel(light.getMaxLevel() == 255? 30 : 255);
    if(!hardOn){
      hardOn = true;
      light.setStat(StatLed::ON);
    }
  }
}
//************************************
Button button(PIN_BUTTON, fDouble, fLong, fShort);
//************************************
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //.................................
  ir_motion_func();  
  // Serial.println(digitalRead(IR_DATA));
  //.................................
  if(buttonStatus){
    buttonStatus = 0;
    button.press_button();
  }
  //.................................
  unsigned long now = millis();
  if (now - lastMsg > 1000) {                                                                 
    if (lightMeter.measurementReady()) {
      lux = lightMeter.readLightLevel();
    }
    lastMsg = now;
//    Serial.println(lux);
    if(lux < 1. && lightStat){
      client.publish(extLight, "0");
      lightStat = false;
    } else if(lux >= 1. && !lightStat){
      client.publish(extLight, "1");
      lightStat = true;
//      tMotion.setTimer(500);
    }
  }
  light.cycle();
  button.cycle();
  ledBlinkR.cycle();
  ledBlinkG.cycle();
  ledBlinkB.cycle();
}