#define R 3
#define G 6
#define B 10

#define STROB 4   //  Стробоскоп, 5 пин, код передачи O
#define SYREL 5   //  Сирена электронная, 7 пин, код передачи Y
#define SYRPN 7   //  Сирена пневматическая, 9 пин, код передачи P

void setup() {
  pinMode(R,OUTPUT);
  pinMode(G,OUTPUT);
  pinMode(B,OUTPUT);
  analogWrite(R,0);
  analogWrite(G,0);
  analogWrite(B,0);

  pinMode(STROB,OUTPUT);
  pinMode(SYREL,OUTPUT);
  pinMode(SYRPN,OUTPUT);
  digitalWrite(STROB,LOW);
  digitalWrite(SYREL,LOW);
  digitalWrite(SYRPN,LOW);
  Serial.begin(9600);
}

void loop(){
  char Color_Num, Color_Code;
  String c_num = "     ";
  int i = 0, c;
  i = 0;
  if(Serial.available()>0) {
    Color_Code = Serial.read();
    delay(20);
    Color_Num = Serial.read();
    delay(20);  
    while (Color_Num != -1) {
      c_num[i++]=Color_Num;
      Color_Num = Serial.read();
      delay(20);  
      if(int(Color_Num)==10) {
        break;
      }
    }
    c = c_num.toInt();
    switch(Color_Code) {
      case 'R': analogWrite(R,c); Serial.print("Color: "); Serial.print(Color_Code); Serial.print("  Value: "); Serial.println(c); break;
      case 'G': analogWrite(G,c); Serial.print("Color: "); Serial.print(Color_Code); Serial.print("  Value: "); Serial.println(c); break;
      case 'B': analogWrite(B,c); Serial.print("Color: "); Serial.print(Color_Code); Serial.print("  Value: "); Serial.println(c); break;
      case 'O': digitalWrite(STROB,c); Serial.print("Strob: "); Serial.print(Color_Code); Serial.print("  Value: "); Serial.println(c); break;
      case 'Y': digitalWrite(SYREL,c); Serial.print("Syreen EL: "); Serial.print(Color_Code); Serial.print("  Value: "); Serial.println(c); break;
      case 'P': digitalWrite(SYRPN,c); Serial.print("Syren PN: "); Serial.print(Color_Code); Serial.print("  Value: "); Serial.println(c); break;
    }
  }
}
