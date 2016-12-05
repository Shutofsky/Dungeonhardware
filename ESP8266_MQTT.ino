#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// RGB FET
#define R_PIN 15
#define G_PIN 13
#define B_PIN 12

// W FET
#define W1_PIN     14 // STR
#define W2_PIN     4  // LGT

// onboard green LED D1
#define LG_PIN    5

// onboard red LED D2
#define LR_PIN   1

//

String cur_command[3];    // текущая исполняемая команда для каждого из каналов. 0 - RGB, 1 - строб, 2 - свет
String cur_buffer;        // полученная с топика команда
String RGB_seq[16];       // RGB - последовательность
String RGB_time[16];      // времена RGB последвоательности
int RGB_index=0;           // текущее положение в RGB последовательности
int RGB_len=0;             // длина RGB последовательности
char RGB_cycle='C';           // признак закольцовки последовательности
int R_val=0, G_val=0, B_val=0;  // значения RGB для записи в порт
long RGB_ctime=0;
int RGB_ftime = 0;
String STR_seq[16];      // стробоскопы - последовательность
String STR_time[16];        // времена для стробоскопов в последвоательности
int STR_index=0;           // текущее положение в последовательности для стробосокопов
int STR_len=0;             // длина последовательности для стробоскопов
int STR_val=0;
char STR_cycle='C';           // признак закольцовки последовательности
long STR_ctime=0;
int STR_ftime = 0;
String LGT_seq[16];      // большой свет - последовательность
String LGT_time[16];        // времена последвоательности для большо света
int LGT_index=0;           // текущее положение в последовательности большого света
int LGT_len=0;             // длина последовательности для большого света
int LGT_val=0;
char LGT_cycle='C';           // признак закольцовки последовательности
long LGT_ctime=0;
int LGT_ftime = 0;

/*
 Формат команды для устройства:
 
 /канал/режим/значение/значение/значение/значение/значение/значение/признак

  канал:  RGB | STR | LGT
      RGB - цветное освещение
      STR - стробоскопы/сирены
      LGT - большой свет (реле)
  режим:  SLD | SEQ
      SLD - жесткое задание режима
      SEQ - последовательность замен
  значение:
      1/0 для каналов STR и LGT,
      FFFFFF - 6 знаков 16-ричного кода цвета
      время в миллисекундах
  признак:
      S - одиночное выполнение последовательности
      C - повторение последовательности в замкнутом режиме
 
 */


// Update these with values suitable for your network.

const char* ssid = "P2797-24";
const char* password = "z0BcfpHu";
const char* mqtt_server = "192.168.0.103";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

int convertToInt(char upper,char lower)
{
  int uVal = (int)upper;
  int lVal = (int)lower;
  uVal = uVal >64 ? uVal - 55 : uVal - 48;
  uVal = uVal << 4;
  lVal = lVal >64 ? lVal - 55 : lVal - 48;
  return uVal + lVal;
}

long stringToLong(String value) {
  long outLong=0;
  long inLong=1;
  int c = 0;
  int idx=value.length()-1;
  for(int i=0;i<=idx;i++){
    c=(int)value[idx-i];
    outLong+=inLong*(c-48);
    inLong*=10;
  }
  return outLong;
}

void parseCommand(String input_str)
{
  int i, pos1, pos2;   
  String tmp_buf = "", cmd, RGB_color, argument;
  for(i=0;i<3;i++) {
    if(input_str.equals(cur_command[i])) {
      Serial1.println("Command already running");
      return; // Такая команда уже выполняется
    }  
  }
  // Такая команда не выполняется, надо разбирать

  tmp_buf = input_str.substring(9);
  cmd = input_str.substring(5,8);
  Serial1.print("TmpBuf: ");
  Serial1.println(tmp_buf);
  Serial1.print("Cmd: ");
  Serial1.println(cmd);
  if (input_str.indexOf("/RGB/") != -1) {
    // Команда относится к RGB-каналу
    cur_command[0] = input_str; 
    RGB_len = 0; // обнуляем длину последовательности   
    RGB_index = 0; // обнуляем текущее положение в команде - пришла новая
    if (cmd.equals("SLD")) {
      // Режим постоянного горения
      RGB_color = tmp_buf.substring(0,6);
      Serial1.println(RGB_color);
      R_val = convertToInt(RGB_color[0],RGB_color[1]) * 4;
      G_val = convertToInt(RGB_color[2],RGB_color[3]) * 4;
      B_val = convertToInt(RGB_color[4],RGB_color[5]) * 4;
      if (R_val == 1020) {
        R_val = 1023;
      }
      if (G_val == 1020) {
        G_val = 1023;
      }
      if (B_val == 1020) {
        B_val = 1023;
      }
      analogWrite(R_PIN,R_val);
      analogWrite(G_PIN,G_val);
      analogWrite(B_PIN,B_val);
    } else if(cmd.equals("SEQ")) {
//      tmp_buf = tmp_buf.substring(1); // Обрезаем самй первый слэш
      pos1 = 0;
      pos2 = 0;
      Serial1.print("Sequence detected ");
      Serial1.println(tmp_buf.indexOf('/',pos1));
      while(tmp_buf.indexOf('/',pos1) != -1) {
        pos2 = tmp_buf.indexOf('/',pos1 + 1);
        argument = tmp_buf.substring(pos1,pos2);
        pos1 = pos2 +1;
        Serial1.print("Argument 1 SEQ = ");
        Serial1.println(argument);
        RGB_seq[RGB_len] = argument;
        pos2 = tmp_buf.indexOf('/',pos1);
        argument = tmp_buf.substring(pos1,pos2);
        Serial1.print("Argument 2 SEQ = ");
        Serial1.println(argument);
        pos1 = pos2 + 1;
        RGB_time[RGB_len] = argument;
        RGB_len += 1;
      }
      RGB_cycle = tmp_buf[pos1];
      RGB_ctime = millis();
      RGB_ftime = 0;
      Serial1.print("RGB SEQ Len = ");
      Serial1.println(RGB_len);
      for(i=0;i<RGB_len;i++) {
        Serial1.print("RGB_color = ");
        Serial1.println(RGB_seq[i]);
        Serial1.print("RGB_time (msec) = ");
        Serial1.println(RGB_time[i]);
      }
    } else {
      Serial1.print("Wrong command!!! - ");
      Serial1.println(cmd);
    }
  } else if (input_str.indexOf("/STR/") != -1) {
    // Команда относится к стробосокпам и сиренам
    cur_command[1] = input_str; // запоминаем текущую команду для этого канала
    STR_len = 0; // обнуляем длину последовательности
    STR_index = 0; // обнуляем теущий индекс - новая команда
    if (cmd.equals("SLD")) {
      // Режим постоянного горения
      STR_val = tmp_buf[1];
      if (STR_val == '0') {
        digitalWrite(W1_PIN,LOW);
      } else {
        digitalWrite(W1_PIN,HIGH);  
      }
    }  else if(cmd.equals("SEQ")) {
      tmp_buf = tmp_buf.substring(1); // Обрезаем самй первый слэш
      pos1 = 0;
      pos2 = 0;
      while(tmp_buf.indexOf(pos1,'/') != -1) {
        pos2 = tmp_buf.indexOf(pos1 + 1,'/');
        argument = tmp_buf.substring(pos1,pos2 - pos1);
        if (tmp_buf.length() < 2) {
          STR_cycle = tmp_buf[0];
          break;
        }
        pos1 = pos2 + 1;
        STR_seq[STR_len] = argument;
        pos2 = tmp_buf.indexOf(pos1,'/');
        argument = tmp_buf.substring(pos1,pos2 - pos1);
        STR_time[STR_len] = argument;
        STR_len += 1;
      }      
    } else {
      Serial1.print("Wrong command!!! - ");
      Serial1.println(cmd);
    }
  } else if (input_str.indexOf("/LGT/") != -1) {
    // Команда относится к большому свету
    cur_command[2] = input_str; // запоминаем текущую команду для этого канала
    LGT_len = 0; // обнуляем длину последовательности
    LGT_index = 0; // обнуляем теущий индекс - новая команда
    if (cmd.equals("SLD")) {
      // Режим постоянного горения
      LGT_len = 0; // обнуляем длину последовательности
      LGT_val = tmp_buf[1];
      if (LGT_val == '0') {
        digitalWrite(W2_PIN,LOW);
      } else {
        digitalWrite(W2_PIN,HIGH);  
      }
    } else if(cmd.equals("SEQ")) {
      tmp_buf = tmp_buf.substring(1); // Обрезаем самй первый слэш
      LGT_len = 0;
      pos1 = 0;
      pos2 = 0;
      while(tmp_buf.indexOf(pos1,'/') != -1) {
        pos2 = tmp_buf.indexOf(pos1 + 1,'/');
        argument = tmp_buf.substring(pos1,pos2 - pos1);
        if (tmp_buf.length() < 2) {
          LGT_cycle = tmp_buf[0];
          break;
        }
        pos1 = pos2 + 1;
        LGT_seq[LGT_len] = argument;
        pos2 = tmp_buf.indexOf(pos1,'/');
        argument = tmp_buf.substring(pos1,pos2 - pos1);
        LGT_time[LGT_len] = argument;
        LGT_len += 1;    
      }    
    } else {
      Serial1.print("Wrong command!!! - ");
      Serial1.println(cmd);
    }
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial1.println();
  Serial1.print("Connecting to ");
  Serial1.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LR_PIN,HIGH);
    delay(250);
    digitalWrite(LR_PIN,LOW);
    delay(250);
    Serial1.print(".");
  }
  digitalWrite(LR_PIN,HIGH);
  Serial1.println("");
  Serial1.println("WiFi connected");
  Serial1.println("IP address: ");
  Serial1.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  char grnstate = 0, redstate = 0;
  String cmd_buffer = "";
  Serial1.print("Message arrived [");
  Serial1.print(topic);
  Serial1.print("] ");
  for (int i = 0; i < length; i++) {
    Serial1.print((char)payload[i]);
  }
  Serial1.println();
  cmd_buffer = String((char*)payload);
  cmd_buffer[length] = 0; 
  Serial1.println(cmd_buffer);
  parseCommand(cmd_buffer);
}

void reconnect() {
  while (!client.connected()) {
    Serial1.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      digitalWrite(LG_PIN,HIGH);
      Serial1.println("connected");
      // Once connected, publish an announcement...
      client.publish("RGBASK","ASK");
      // ... and resubscribe
      client.subscribe("RGB");
    } else {
      digitalWrite(LG_PIN,LOW);
      Serial1.print("failed, rc=");
      Serial1.print(client.state());
      Serial1.println(" try again in 5 seconds");
      // Wait 1 seconds before retrying
      delay(1000);
    }
  }
}

void setup() {
  pinMode(R_PIN, OUTPUT);     
  pinMode(G_PIN, OUTPUT);     
  pinMode(B_PIN, OUTPUT);     
  pinMode(W1_PIN, OUTPUT);     
  pinMode(W2_PIN, OUTPUT);     
  pinMode(LG_PIN, OUTPUT);     
  pinMode(LR_PIN, OUTPUT);     
  
  Serial1.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
}

void loop() {

  long RGB_now = 0, STR_now = 0, LGT_now = 0, fromrand, torand, pos1, pos2;
  String tmp1,tmp2;

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();

  if(RGB_len > 0) {
    if(RGB_index >= RGB_len) {
      if(RGB_cycle == 'S') {
        RGB_index = 0;
        RGB_ftime = 0;
      } else {
        RGB_index = 0;
        RGB_len = 0;
        RGB_ftime = 1;
      }
    } 
    if(RGB_ftime == 0) {
      Serial1.println("Setting time on phase");
      if(RGB_time[RGB_index].indexOf(',') != -1) {
        // генерим рандомную величину
        pos1 = RGB_time[RGB_index].indexOf(',');
        tmp1 = RGB_time[RGB_index].substring(0,pos1);
        tmp2 = RGB_time[RGB_index].substring(pos1+1);
        Serial1.print("Tmp1 =");
        Serial1.println(tmp1);
        Serial1.print("Tmp2 =");
        Serial1.println(tmp2);
        fromrand = stringToLong(tmp1);
        torand = stringToLong(tmp2);
        RGB_now = random(fromrand, torand);
      } else {
        tmp1 = RGB_time[RGB_index];
        RGB_now = stringToLong(tmp1);
      }
      RGB_ftime = 1;
      RGB_ctime = millis() + RGB_now;
      R_val = convertToInt(RGB_seq[RGB_index][0],RGB_seq[RGB_index][1]) * 4;
      G_val = convertToInt(RGB_seq[RGB_index][2],RGB_seq[RGB_index][3]) * 4;
      B_val = convertToInt(RGB_seq[RGB_index][4],RGB_seq[RGB_index][5]) * 4;
      if (R_val == 1020) {
        R_val = 1023;
      }
      if (G_val == 1020) {
        G_val = 1023;
      }
      if (B_val == 1020) {
        B_val = 1023;
      }
      analogWrite(R_PIN,R_val);
      analogWrite(G_PIN,G_val);
      analogWrite(B_PIN,B_val);
        
      Serial1.print("Millis for phase = ");
      Serial1.println(RGB_ctime);
      Serial1.print("Millis now = ");
      Serial1.println(now);
      
    }  
    if(RGB_ctime <= now) {
      Serial1.print("Now = ");
      Serial1.println(now);
      // меняем фазу
      RGB_index += 1;
      RGB_ftime = 0;
      Serial1.println("Changed phase.");
      Serial1.print("RGB_index = ");
      Serial1.println(RGB_index);
    }
  }

  if(STR_len > 0) {
    if(STR_index >= STR_len) {
      if(STR_cycle == 'S') {
        STR_index = 0;
        STR_ftime = 0;
      } else {
        STR_index = 0;
        STR_len = 0;
        STR_ftime = 1;
      }
    } 
    if(STR_ftime == 0) {
      Serial1.println("Setting time on phase");
      if(STR_time[STR_index].indexOf(',') != -1) {
        // генерим рандомную величину
        pos1 = STR_time[STR_index].indexOf(',');
        tmp1 = STR_time[STR_index].substring(0,pos1);
        tmp2 = STR_time[STR_index].substring(pos1+1);
        Serial1.print("Tmp1 =");
        Serial1.println(tmp1);
        Serial1.print("Tmp2 =");
        Serial1.println(tmp2);
        fromrand = stringToLong(tmp1);
        torand = stringToLong(tmp2);
        STR_now = random(fromrand, torand);
      } else {
        tmp1 = STR_time[STR_index];
        STR_now = stringToLong(tmp1);
      }
      STR_ftime = 1;
      STR_ctime = millis() + STR_now;

      if(STR_seq[STR_index].equals("0")) {        
        digitalWrite(W1_PIN,LOW);
      } else {
        digitalWrite(W1_PIN,HIGH);
      }
      Serial1.print("Millis for phase = ");
      Serial1.println(STR_ctime);
      Serial1.print("Millis now = ");
      Serial1.println(now);
    }  
    if(STR_ctime <= now) {
      Serial1.print("Now = ");
      Serial1.println(now);
      // меняем фазу
      STR_index += 1;
      STR_ftime = 0;
      Serial1.println("Changed phase.");
      Serial1.print("RGB_index = ");
      Serial1.println(STR_index);
    }
  }

  if(LGT_len > 0) {
    if(LGT_index >= LGT_len) {
      if(LGT_cycle == 'S') {
        LGT_index = 0;
        LGT_ftime = 0;
      } else {
        LGT_index = 0;
        LGT_len = 0;
        LGT_ftime = 1;
      }
    } 
    if(LGT_ftime == 0) {
      Serial1.println("Setting time on phase");
      if(LGT_time[LGT_index].indexOf(',') != -1) {
        // генерим рандомную величину
        pos1 = LGT_time[LGT_index].indexOf(',');
        tmp1 = LGT_time[LGT_index].substring(0,pos1);
        tmp2 = LGT_time[LGT_index].substring(pos1+1);
        Serial1.print("Tmp1 =");
        Serial1.println(tmp1);
        Serial1.print("Tmp2 =");
        Serial1.println(tmp2);
        fromrand = stringToLong(tmp1);
        torand = stringToLong(tmp2);
        LGT_now = random(fromrand, torand);
      } else {
        tmp1 = LGT_time[LGT_index];
        LGT_now = stringToLong(tmp1);
      }
      LGT_ftime = 1;
      LGT_ctime = millis() + LGT_now;

      if(LGT_seq[LGT_index].equals("0")) {        
        digitalWrite(W2_PIN,LOW);
      } else {
        digitalWrite(W2_PIN,HIGH);
      }
      Serial1.print("Millis for phase = ");
      Serial1.println(LGT_ctime);
      Serial1.print("Millis now = ");
      Serial1.println(now);
    }  
    if(LGT_ctime <= now) {
      Serial1.print("Now = ");
      Serial1.println(now);
      // меняем фазу
      LGT_index += 1;
      LGT_ftime = 0;
      Serial1.println("Changed phase.");
      Serial1.print("LGT_index = ");
      Serial1.println(LGT_index);
    }
  }
}

