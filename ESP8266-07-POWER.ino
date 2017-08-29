#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define REL_IN 16    // HIGH by default
#define REL_PWR 14   // LOW by default
#define KBD_IN 4     // HIGH by default
#define FAN_PWR 13   // LOW by default
#define KBD_PWR 12   // LOW by default

#define CLAC_ON   digitalWrite(REL_PWR, LOW)
#define CLAC_OFF   digitalWrite(REL_PWR, HIGH)

const char* ssid = "ArmyDep";
const char* password = "z0BcfpHu";
const char* mqtt_server = "192.168.0.200";

int mainMode = 0; // 0 - выключено, нужно вспомогательное питание. 1 - включено вспомогательное питание, ждем запуска реактора. 2 - реакто включен
int reqMode = 0;

unsigned long int kbPressTime = 0;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void parseCommand(String input_str)
{
  int i, pos1, pos2;   
  String tmp_buf = "", cmd, RGB_color, argument;

  if (input_str.indexOf("/AUX/") != -1) {
    // Включено вспомогательбное питание
    reqMode = 1;
  } else if (input_str.indexOf("/OFF/") != -1) {
    // Всё выключить
    reqMode = 0;
  } else if (input_str.indexOf("/PWR/") != -1) {
    // Запустить реактор
    reqMode = 2;
  }
}

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
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
}

void callback(char* topic, byte* payload, unsigned int length) {
  char grnstate = 0, redstate = 0;
  String cmd_buffer = "";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  cmd_buffer = String((char*)payload);
  cmd_buffer[length] = 0; 
  Serial.println(cmd_buffer);
  parseCommand(cmd_buffer);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("PWRASK","ASK");
      // ... and resubscribe
      client.subscribe("PWR");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 1 seconds before retrying
      delay(1000);
    }
  }
}

void setup() {
  
  pinMode(KBD_IN, INPUT);     
  pinMode(REL_IN, INPUT);     
  
  pinMode(FAN_PWR, OUTPUT);  
  digitalWrite(FAN_PWR, LOW);
  pinMode(KBD_PWR, OUTPUT);  
  digitalWrite(KBD_PWR, LOW);
  pinMode(REL_PWR, OUTPUT);     
  CLAC_ON;
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  int relInp = 0, kbdInp = 0;
  relInp = digitalRead(REL_IN);
  kbdInp = digitalRead(KBD_IN);
  if (relInp == 0 and mainMode == 0) {
    Serial.println("Relay");
    reqMode = 1;
  } else if (kbdInp == 0 and mainMode == 1) {
    if (kbPressTime == 0) {
      kbPressTime = millis();
    } else if ((kbPressTime+1000)<=millis()) {
      kbPressTime = 0;
      reqMode = 2;
    }
  } else if (kbdInp == 1) {
    kbPressTime = 0;
  }
  if (reqMode != mainMode) {
    // Смена режима
    if (mainMode == 0) { // Были выключены
      if (reqMode == 1) { // Подано вспомогательное питание
        Serial.println("Battery connected!");
        digitalWrite(FAN_PWR, HIGH);
        delay(2000);
        CLAC_OFF;
        delay(1000);
        CLAC_ON;
        delay(2000);
        digitalWrite(FAN_PWR, LOW);
        CLAC_OFF;
        delay(2000);
        digitalWrite(FAN_PWR, HIGH);
        CLAC_ON;
        delay(2000);
        CLAC_OFF;
        delay(2000);
        digitalWrite(KBD_PWR, HIGH);
        client.publish("PWRASK","AUX");
        delay(5000);
        digitalWrite(FAN_PWR, LOW);
      }
      else if (reqMode == 2) {
        digitalWrite(FAN_PWR, LOW);
        digitalWrite(KBD_PWR, HIGH);
        client.publish("PWRASK","PWR");
      }
    } 
    else if (mainMode == 1) { // Активно вcпомогательное питание
      if (reqMode == 0) { // Отключаем
        CLAC_ON;
        digitalWrite(FAN_PWR, LOW);
        digitalWrite(KBD_PWR, LOW);
        client.publish("PWRASK","OFF");
      }
      else if (reqMode == 2) { // Запускаем реактор
        Serial.println("Started reactor!");
        digitalWrite(FAN_PWR, HIGH);
        digitalWrite(KBD_PWR, HIGH);
        client.publish("PWRASK","PWR");
      }
    }
    else if (mainMode == 2) { // Активно основное питание
      if (reqMode == 0) { // Отключаем
        CLAC_ON;
        digitalWrite(FAN_PWR, LOW);
        digitalWrite(KBD_PWR, LOW);
        client.publish("PWRASK","OFF");
      }
    }
    mainMode = reqMode;
  }
}


