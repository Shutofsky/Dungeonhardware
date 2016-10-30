#define R1_D0 4
#define R1_D1 3
#define R2_D0 6
#define R2_D1 5


#define IN3V 7
#define OUT5V 8

#define DELAYKB 20

#include "pins_arduino.h"

void setup()
{
 Serial.begin(9600);
 Serial.println("Starting!");
 pinMode(R1_D0,INPUT);
 digitalWrite(R1_D0,HIGH);
 pinMode(R1_D1,INPUT);
 digitalWrite(R1_D1,HIGH);
 pinMode(R2_D0,INPUT);
 digitalWrite(R2_D0,HIGH);
 pinMode(R2_D1,INPUT);
 digitalWrite(R2_D1,HIGH);
 pinMode(IN3V,INPUT);
 digitalWrite(IN3V,HIGH);
 pinMode(OUT5V,OUTPUT);
 digitalWrite(OUT5V,HIGH);
 Serial.println("Ready!");
}

int r1_d0_s = HIGH, r1_d1_s = HIGH, r1_d0_f = 0, r1_d1_f = 0;
int r2_d0_s = HIGH, r2_d1_s = HIGH, r2_d0_f = 0, r2_d1_f = 0;
byte r1_b = 0, r1_cnt = 0, r1_card[26] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} ;
byte r2_b = 0, r2_cnt = 0, r2_card[26] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} ;
char r1_out[2], r2_out[2];
unsigned long int r1_timeout;
unsigned long int r2_timeout;

void loop() 
{
  r1_d0_s = digitalRead(R1_D0);
  r1_d1_s = digitalRead(R1_D1);

  r2_d0_s = digitalRead(R2_D0);
  r2_d1_s = digitalRead(R2_D1);

  digitalWrite(OUT5V,digitalRead(IN3V));

  if(r1_d0_s == HIGH) {
    r1_d0_f = 0;
  }

  if(r1_d1_s == HIGH) {
    r1_d1_f = 0;
  }
    
  if(r1_d0_s == LOW) {    // Поймали ввод со считывателя 1 d0
    if(r1_d0_f == 0) {    // Статус не изменялся
      r1_timeout = millis();
      r1_d0_f = 1;        // Начали читать
      if(r1_d1_s == HIGH) {
        r1_card[r1_cnt] = 0 ;
        r1_d1_f = 0;
        r1_cnt += 1;
        r1_b = r1_b << 1;
      }
    }
  }
  
  if(r1_d1_s == LOW) {    // Поймали ввод со считывателя 1 d1
    if(r1_d1_f == 0) {    // Статус не изменялся
      r1_timeout = millis();
      r1_d1_f = 1;        // Начали читать
      if(r1_d0_s == HIGH) {
        r1_card[r1_cnt] = 1 ;
        r1_d0_f = 0;
        r1_cnt += 1;
        r1_b = (r1_b << 1) | 0x01;
      }
    }
  }


  if(r2_d0_s == HIGH) {
    r2_d0_f = 0;
  }

  if(r2_d1_s == HIGH) {
    r2_d1_f = 0;
  }
    
  if(r2_d0_s == LOW) {    // Поймали ввод со считывателя 2 d0
    if(r2_d0_f == 0) {    // Статус не изменялся
      r2_timeout = millis();
      r2_d0_f = 1;        // Начали читать
      if(r2_d1_s == HIGH) {
        r2_card[r2_cnt] = 0 ;
        r2_d1_f = 0;
        r2_cnt += 1;
        r2_b = r2_b << 1;
      }
    }
  }
  
  if(r2_d1_s == LOW) {    // Поймали ввод со считывателя 2 d1
    if(r2_d1_f == 0) {    // Статус не изменялся
      r2_timeout = millis();
      r2_d1_f = 1;        // Начали читать
      if(r2_d0_s == HIGH) {
        r2_card[r2_cnt] = 1 ;
        r2_d0_f = 0;
        r2_cnt += 1;
        r2_b = (r2_b << 1) | 0x01;
      }
    }
  }

  if(r1_cnt == 4) {
    if((millis() - r1_timeout) >= DELAYKB) {  //  Нажата кнопка
      Serial.print("R1KB");
      Serial.println(r1_b);
      r1_b = 0;
      r1_cnt = 0;
      r1_timeout=millis();
    }
  } else if (r1_cnt == 26) {                     //  Считываем карточку
    Serial.print("R1CD");
    r1_b = 0;
    for(int i=1; i <= 24; i++) {
      r1_b = (r1_b << 1) | r1_card[i];
      if(i%8 == 0) {
        sprintf(r1_out,"%02X",r1_b);
        Serial.print(r1_out);
      }
    }
    Serial.println("");
    r1_b = 0;
    r1_cnt = 0;
    r1_timeout=millis();
  }


  if(r2_cnt == 4) {
    if((millis() - r2_timeout) >= DELAYKB) {  //  Нажата кнопка
      Serial.print("R2KB");
      Serial.println(r2_b);
      r2_b = 0;
      r2_cnt = 0;
      r2_timeout=millis();
    }
  } else if (r2_cnt == 26) {                     //  Считываем карточку
    Serial.print("R2CD");
    for(int i=1; i <= 24; i++) {
      r2_b = (r2_b << 1) | r2_card[i];
      if(i%8 == 0) {
        sprintf(r2_out,"%02X",r2_b);
        Serial.print(r2_out);
      }
    }
    Serial.println("");
    r2_b = 0;
    r2_cnt = 0;
    r2_timeout=millis();
  }
  
}
