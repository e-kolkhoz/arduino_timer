
#define ENC_CLK 2
#define ENC_DT 3
#define ENC_SW 4

#define RELAY 13                                             // relay pin (handy, with blue led)

#define BEEP 5                                               // speaker pin

#include "GyverEncoder.h"                                    // https://github.com/AlexGyver/GyverLibs/releases/download/GyverEncoder/GyverEncoder.zip
#include <iarduino_OLED_txt.h>                               // https://github.com/tremaru/iarduino_OLED_txt/archive/1.1.0.zip

iarduino_OLED_txt myOLED(0x3C);                              // Объявляем объект myOLED, указывая адрес дисплея на шине I2C: 0x3C или 0x3D.
                                                           
extern uint8_t MediumFontRus[];                              // display font

Encoder enc(ENC_CLK, ENC_DT, ENC_SW);

const uint32_t WORK_TIME = 3600000;                     // delay time switching off the relay (millis)
const uint32_t BEFORE_BEEP_TIME = 3540000;              // delay time before beeping (millis)

const String PASSW = "1337";                           
uint32_t turn_off_millis;
uint32_t beep_millis;
uint32_t last_check_millis;


enum {
  SHOW_TIMER, // режим показа таймера
  SHOW_MENU,
  AUTH, // режим аутентификации
  SET_PASSW,
  SET_ON_TIME,
  SET_BEEP_TIME,
  CONFIRM
} mode;

bool beeping;

uint8_t pasw_char_pos;
uint8_t pasw_char_val;
uint8_t inp_passw[4]; 

void beep(){
   tone(BEEP, 1000, 200); // Запустили звучание
   beeping = true;
   //Serial.println("beep!");
}

void beep_err(){
   tone(BEEP, 300, 200); // Запустили звучание
   delay(200);
   noTone(BEEP); // Остановили звучание
}

void no_beep(){
   noTone(BEEP); // Остановили звучание
   beeping = false;
   //Serial.println("no beep");
}


void setup() {
  //Serial.begin(9600);
  myOLED.begin();                                          // Инициируем работу с дисплеем.
  myOLED.setFont(MediumFontRus);                           // Указываем шрифт который требуется использовать для вывода цифр и текста.

  
  attachInterrupt(0, enc_check, CHANGE);    // прерывание на 2 пине! CLK у энка
  attachInterrupt(1, enc_check, CHANGE);     // прерывание на 3 пине! DT у энка
  enc.setType(TYPE2);
  pinMode(RELAY, OUTPUT);
  turn_off();
}

String passw2str(uint8_t* passw){
  String str = "";
  
  for (uint8_t i = 0; i < 4; i++){
    if(passw[3-i] > 9) str+='X'; else str+=String(passw[3-i]);
    }
  return str;
  }

void check_passw(String passw){
  if(passw == PASSW){
    no_beep();
    restart();    
  }else{
    beep_err();
    turn_off();  
  }
}


void check_timer(){
  uint32_t diff = millis() - last_check_millis;
  /*Serial.println(millis());
  Serial.println(turn_off_millis);
  Serial.println(diff);
  Serial.println(last_check_millis);*/
  last_check_millis = millis();
  if(turn_off_millis < diff){
     turn_off_millis = 0;  
  }else{
    turn_off_millis-=diff;
  };
  if(beep_millis < diff){
     beep_millis = 0;  
  }else{
    beep_millis-=diff;
  };
  if(beeping && beep_millis == 0){
    beep();    
  }
  if(turn_off_millis == 0){
    turn_off();
  }
  
}

void show_timer() {
   String minutes = String(turn_off_millis/60000);
   String seconds = String((turn_off_millis % 60000) / 1000);
   if(minutes.length()==1) minutes = "0" + minutes;
   if(seconds.length()==1) seconds = "0" + seconds;
   String value = minutes + ":" + seconds;
   //Serial.println(value);


   myOLED.setCursor(16,4);
   myOLED.print(value);  
}

void turn_off(){
  mode = AUTH;
  digitalWrite(RELAY, LOW);
  pasw_char_pos = 0;
  pasw_char_val = 0;
  myOLED.clrScr();
  for (uint8_t i = 0; i < 4; i++) inp_passw[i] = 255;
  }

void restart(){
  digitalWrite(RELAY, HIGH);
  last_check_millis = millis();
  turn_off_millis = WORK_TIME;
  beep_millis = BEFORE_BEEP_TIME;
  mode = SHOW_TIMER;
  beeping = true;
  pasw_char_pos = 0;
  pasw_char_val = 0;
  for (uint8_t i = 0; i < 4; i++) inp_passw[i] = 255;
  }

void enc_check(){
  // отработка в прерывании  
  enc.tick();
}
void enc_action(){
  // отработка в прерывании  
  enc.tick();
  if(enc.isClick()) isrCLK();
  else{ // если был поворот
      if (enc.isRight() || enc.isRightH()) {
        isrDT(false);        
      }
      if (enc.isLeft() || enc.isLeftH()) {
        isrDT(true);
      }    
    }  
  }

void isrCLK() {
  //Serial.println("click");
  switch (mode) {
    case AUTH:
      //Serial.println("mode AUTH");
      if(pasw_char_pos <= 2){
        pasw_char_pos++;
        pasw_char_val = 0;
      }else{
        pasw_char_pos = 0;
        pasw_char_val = 0;
        check_passw(passw2str(inp_passw));
      }
      break;
    case SHOW_TIMER:
      if(beeping) no_beep();
      //Serial.println("mode SHOW_TIMER");
      break;
    default:
      break;
  }
}
void isrDT(bool left) {
  switch (mode) {
    case AUTH:
      //Serial.println("mode AUTH");
      if (left) {
        //Serial.println("Left");
        if(pasw_char_val<=0) pasw_char_val = 9;
        else pasw_char_val-= 1;
      }else{
        //Serial.println("Right");
        pasw_char_val = (pasw_char_val + 1) % 10;
      }
      inp_passw[3-pasw_char_pos] = pasw_char_val;
      //Serial.println(passw2str(inp_passw));
      myOLED.setCursor(16,4);
      myOLED.print(passw2str(inp_passw)); 

      break;
    case SHOW_TIMER:
      if(beeping) no_beep();
      //Serial.println("mode SHOW_TIMER");
      break;
    default:
      break;
  }  
}

void loop() {
  enc_action();
  switch (mode) {
   case SHOW_TIMER:
     show_timer();
     check_timer();
     delay(500);
     break;
  }
}
