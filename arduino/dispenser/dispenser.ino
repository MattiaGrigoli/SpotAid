#include <pitches.h>
#include <dht_nonblocking.h>
#include "LoRa_E220.h"
#include <LiquidCrystal_I2C.h>
#include <SR04.h>
#include <avr/pgmspace.h>

//IR memory optimizations
#define RAW_BUFFER_LENGHT 50
#define EXCLUDE_EXOTIC_PROTOCOLS
#define DECODE_NEC
#define NO_LED_FEEDBACK_CODE

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

const uint16_t melody[] PROGMEM = {
  NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6};
uint8_t duration = 500;

SR04 sr04 = SR04(USECHO, USTRIG);

IRrecv irrecv(IRPIN);
uint32_t last_decodedRawData = 0;

LoRa_E220 e220ttl(LORARX, LORATX);

void setup() {
  // put your setup code here, to run once:
  DEBUG_PRINTLN(F("--- DEBUG ENABLED ---"));
  Serial.begin(9600);

  DEBUG_PRINTLN(F("initialize DC motor"));
  pinMode(DCENABLE, OUTPUT);
  pinMode(DCDIRA, OUTPUT);
  pinMode(DCDIRB, OUTPUT);

  DEBUG_PRINTLN(F("initialize LCD I2C"));
  lcd.init();
  lcd.clear();
  lcd.backlight();

  DEBUG_PRINTLN(F("Initialize IR receiver"));
  irrecv.enableIRIn();

  DEBUG_PRINTLN(F("initialize lora module"));
  e220ttl.begin();

#ifdef DEBUG_ENABLE
  //TestErogate();
  //testLcd();
  //testBuz();
  //testUR();
  ResponseStructContainer c;
  c = e220ttl.getConfiguration();
  // It's important get configuration pointer before all other operation
  Configuration configuration = *(Configuration*) c.data;
  Serial.println(c.status.getResponseDescription());
  Serial.println(c.status.code);

  printParameters(configuration);
  c.close();
#endif

}

void loop() {
  // put your main code here, to run repeatedly:
#ifdef DEBUG_ENABLE
  //testMic();
  /*if (irrecv.decode()) // have we received an IR signal?
  {
    translateIR();
    irrecv.resume(); // receive the next value
  }*/
#endif
}

#ifdef DEBUG_ENABLE
void TestErogate(){
  DEBUG_PRINTLN(F("starting rotating"));
  analogWrite(DCENABLE, 180);
  digitalWrite(DCDIRA, HIGH);
  digitalWrite(DCDIRB, LOW);
  delay(3000);
  DEBUG_PRINTLN(F("stopping rotating"));
  digitalWrite(DCENABLE,LOW);
}

void testLcd(){
  lcd.setCursor(0,0);
  lcd.print(F("funziono!"));
  lcd.setCursor(0,1);
  lcd.print(F("qui pure"));
}

void testBuz(){
  tone(BUZPIN, pgm_read_word_near(&melody[0]), duration);
}

void testUR(){
  DEBUG_PRINTLN(sr04.Distance()); //cm
}

//is it really working?
void testMic(){
  DEBUG_PRINTLN(analogRead(MICPIN)); //int
}

void translateIR() // takes action based on IR code received
{
  // Check if it is a repeat IR code 
  if (irrecv.decodedIRData.flags)
  {
    //set the current decodedRawData to the last decodedRawData 
    irrecv.decodedIRData.decodedRawData = last_decodedRawData;
    Serial.println(F("REPEAT!"));
  } else
  {
    //output the IR code on the serial monitor
    Serial.print(F("IR code:0x"));
    Serial.println(irrecv.decodedIRData.decodedRawData, HEX);
  }
  //map the IR code to the remote key
  switch (irrecv.decodedIRData.decodedRawData)
  {
    case 0xBA45FF00: Serial.println(F("POWER")); break;
    case 0xB847FF00: Serial.println(F("FUNC/STOP")); break;
    case 0xB946FF00: Serial.println(F("VOL+")); break;
    case 0xBB44FF00: Serial.println(F("FAST BACK"));    break;
    case 0xBF40FF00: Serial.println(F("PAUSE"));    break;
    case 0xBC43FF00: Serial.println(F("FAST FORWARD"));   break;
    case 0xF807FF00: Serial.println(F("DOWN"));    break;
    case 0xEA15FF00: Serial.println(F("VOL-"));    break;
    case 0xF609FF00: Serial.println(F("UP"));    break;
    case 0xE619FF00: Serial.println(F("EQ"));    break;
    case 0xF20DFF00: Serial.println(F("ST/REPT"));    break;
    case 0xE916FF00: Serial.println(F("0"));    break;
    case 0xF30CFF00: Serial.println(F("1"));    break;
    case 0xE718FF00: Serial.println(F("2"));    break;
    case 0xA15EFF00: Serial.println(F("3"));    break;
    case 0xF708FF00: Serial.println(F("4"));    break;
    case 0xE31CFF00: Serial.println(F("5"));    break;
    case 0xA55AFF00: Serial.println(F("6"));    break;
    case 0xBD42FF00: Serial.println(F("7"));    break;
    case 0xAD52FF00: Serial.println(F("8"));    break;
    case 0xB54AFF00: Serial.println(F("9"));    break;
    default:
      Serial.println(F(" other button   "));
  }// End Case
  DEBUG_PRINTLN(irrecv.decodedIRData.protocol);
  //store the last decodedRawData
  last_decodedRawData = irrecv.decodedIRData.decodedRawData;
  delay(500); // Do not get immediate repeat
} //END translateIR

void printParameters(struct Configuration configuration) {
  Serial.println(F("----------------------------------------"));

  Serial.print(F("HEAD : "));  Serial.print(configuration.COMMAND, HEX);Serial.print(F(" "));Serial.print(configuration.STARTING_ADDRESS, HEX);Serial.print(F(" "));Serial.println(configuration.LENGHT, HEX);
  Serial.println(F(" "));
  Serial.print(F("AddH : "));  Serial.println(configuration.ADDH, HEX);
  Serial.print(F("AddL : "));  Serial.println(configuration.ADDL, HEX);
  Serial.println(F(" "));
  Serial.print(F("Chan : "));  Serial.print(configuration.CHAN, DEC); Serial.print(F(" -> ")); Serial.println(configuration.getChannelDescription());
  Serial.println(F(" "));
  Serial.print(F("SpeedParityBit     : "));  Serial.print(configuration.SPED.uartParity, BIN);Serial.print(F(" -> ")); Serial.println(configuration.SPED.getUARTParityDescription());
  Serial.print(F("SpeedUARTDatte     : "));  Serial.print(configuration.SPED.uartBaudRate, BIN);Serial.print(F(" -> ")); Serial.println(configuration.SPED.getUARTBaudRateDescription());
  Serial.print(F("SpeedAirDataRate   : "));  Serial.print(configuration.SPED.airDataRate, BIN);Serial.print(F(" -> ")); Serial.println(configuration.SPED.getAirDataRateDescription());
  Serial.println(F(" "));
  Serial.print(F("OptionSubPacketSett: "));  Serial.print(configuration.OPTION.subPacketSetting, BIN);Serial.print(F(" -> ")); Serial.println(configuration.OPTION.getSubPacketSetting());
  Serial.print(F("OptionTranPower    : "));  Serial.print(configuration.OPTION.transmissionPower, BIN);Serial.print(F(" -> ")); Serial.println(configuration.OPTION.getTransmissionPowerDescription());
  Serial.print(F("OptionRSSIAmbientNo: "));  Serial.print(configuration.OPTION.RSSIAmbientNoise, BIN);Serial.print(F(" -> ")); Serial.println(configuration.OPTION.getRSSIAmbientNoiseEnable());
  Serial.println(F(" "));
  Serial.print(F("TransModeWORPeriod : "));  Serial.print(configuration.TRANSMISSION_MODE.WORPeriod, BIN);Serial.print(F(" -> ")); Serial.println(configuration.TRANSMISSION_MODE.getWORPeriodByParamsDescription());
  Serial.print(F("TransModeEnableLBT : "));  Serial.print(configuration.TRANSMISSION_MODE.enableLBT, BIN);Serial.print(F(" -> ")); Serial.println(configuration.TRANSMISSION_MODE.getLBTEnableByteDescription());
  Serial.print(F("TransModeEnableRSSI: "));  Serial.print(configuration.TRANSMISSION_MODE.enableRSSI, BIN);Serial.print(F(" -> ")); Serial.println(configuration.TRANSMISSION_MODE.getRSSIEnableByteDescription());
  Serial.print(F("TransModeFixedTrans: "));  Serial.print(configuration.TRANSMISSION_MODE.fixedTransmission, BIN);Serial.print(F(" -> ")); Serial.println(configuration.TRANSMISSION_MODE.getFixedTransmissionDescription());

  Serial.println(F("----------------------------------------"));
}
#endif