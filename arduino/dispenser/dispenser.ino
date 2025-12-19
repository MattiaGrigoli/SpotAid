#include <pitches.h>
#include <dht_nonblocking.h>
#include "LoRa_E220.h"
#include <LiquidCrystal_I2C.h>
#include <SR04.h>
#include <IRremote.h>

//to enable and disable DEBUG
#define DEBUG_ENABLE
#ifdef DEBUG_ENABLE
  // Se il debug è attivo, le macro fanno una stampa
  #define DEBUG_PRINT(x)       Serial.print(x)
  #define DEBUG_PRINTLN(x)     Serial.println(x)
#else
  // Se il debug è disattivo, le macro sono definite come 'nulla' (vengono ignorate)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

#define DHTPIN 2
#define DHTTYPE DHT11
#define IRPIN 11 //infrared
#define MICPIN A0
#define BUZPIN 8
#define DCENABLE 5
#define DCDIRA 3
#define DCDIRB 4
#define USECHO 6 //ultrasuond
#define USTRIG 7 //ultrasound
#define LORATX 9
#define LORARX 10

// I2C display A4 e A5
LiquidCrystal_I2C lcd(0x27, 16, 2);

int melody[] = {
  NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6};
int duration = 500;

SR04 sr04 = SR04(USECHO, USTRIG);

void setup() {
  // put your setup code here, to run once:
  DEBUG_PRINTLN("--- DEBUG ENABLED ---");
  Serial.begin(9600);

  DEBUG_PRINTLN("initialize DC motor");
  pinMode(DCENABLE, OUTPUT);
  pinMode(DCDIRA, OUTPUT);
  pinMode(DCDIRB, OUTPUT);

  DEBUG_PRINTLN("initialize LCD I2C");
  lcd.init();
  lcd.clear();
  lcd.backlight();

#ifdef DEBUG_ENABLE
  //TestErogate();
  //testLcd();
  //testBuz();
  //testUR();
#endif

}

void loop() {
  // put your main code here, to run repeatedly:
#ifdef DEBUG_ENABLE
  //testMic();
#endif
}

#ifdef DEBUG_ENABLE
void TestErogate(){
  DEBUG_PRINTLN("starting rotating");
  analogWrite(DCENABLE, 180);
  digitalWrite(DCDIRA, HIGH);
  digitalWrite(DCDIRB, LOW);
  delay(3000);
  DEBUG_PRINTLN("stopping rotating");
  digitalWrite(DCENABLE,LOW);
}

void testLcd(){
  lcd.setCursor(0,0);
  lcd.print("funziono!");
  lcd.setCursor(0,1);
  lcd.print("qui pure");
}

void testBuz(){
  tone(BUZPIN, melody[0], duration);
}

void testUR(){
  DEBUG_PRINTLN(sr04.Distance()); //cm
}

//is it really working?
void testMic(){
  DEBUG_PRINTLN(analogRead(MICPIN)); //int
}
#endif