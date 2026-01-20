#include <DHT.h>
#include <DHT_U.h>
#include <dht_nonblocking.h>
#include <LoRa_E220.h>
#include <pitches.h>
#include <LiquidCrystal_I2C.h>
#include <pgmspace.h>
#include <Wire.h>

//From https://iotdesignpro.com/projects/wireless-communication-between-arduino-and-raspberry-pi-using-lora-module-sx1278
// used for SPI communication between arduino and LoRa modules
#include <SPI.h>

//IR memory optimizations
#define RAW_BUFFER_LENGHT 50
#define EXCLUDE_EXOTIC_PROTOCOLS
#define DECODE_NEC
#define NO_LED_FEEDBACK_CODE

#include <IRremote.hpp>

// Receiver address & channel
uint8_t TARGET_ADDH = 0x00;
uint8_t TARGET_ADDL = 0x02;
uint8_t TARGET_CHANNEL = 23;

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

//hardware constants
#define DHTTYPE DHT11
#define DHTPIN 18
#define IRPIN 4 //infrared
#define MICPIN 34
#define BUZPIN 25
#define DCDIRA 14
#define DCDIRB 13
//serial2
#define LORATX 16 //tx module side
#define LORARX 17 //rc module side
#define LORAAUX 15
#define LORAM0 26
#define LORAM1 27
#define BUTTON 35

// I2C display A4 e A5
LiquidCrystal_I2C lcd(0x27, 16, 2);
const uint16_t melody[] PROGMEM = {
  NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6};
uint8_t duration = 500;
IRrecv irrecv(IRPIN);
uint32_t last_decodedRawData = 0;
LoRa_E220 e220ttl(&Serial2, LORAAUX, LORAM0, LORAM1);
DHT dht (DHTPIN, DHTTYPE);

//logic constants
#define TEMPMIN -7
#define TEMPMAX 30
#define HUMMIN 5
#define HUMMAX 85
#define MICMAX 1000 //ESP max 4095
#define IDMACHINE 1 //change for every dispenser

typedef enum 
{
  IDLE,
  EROGATION,
  ALERT
} State;

typedef enum
{
  POWER,
  FUNC,
  VOLUP,
  FBACK,
  PAUSE,
  FFORW,
  DOWN,
  VOLDOWN,
  UP,
  EQ,
  ST,
  ZERO,
  ONE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  SEVEN,
  EIGHT,
  NINE,
  ERROR,
  NONE
} IRButton;

State state;
IRButton button;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  DEBUG_PRINTLN(F("--- DEBUG ENABLED ---"));

  DEBUG_PRINTLN(F("initialize DC motor"));
  pinMode(DCDIRA, OUTPUT);
  pinMode(DCDIRB, OUTPUT);

  DEBUG_PRINTLN(F("initialize LCD I2C"));
  lcd.init();
  lcd.clear();
  lcd.backlight();

  DEBUG_PRINTLN(F("Initialize IR receiver"));
  irrecv.enableIRIn();

  DEBUG_PRINTLN(F("initialize DHT module module"));
  dht.begin();

  DEBUG_PRINTLN(F("initialize lora module"));
  Serial2.begin(9600, SERIAL_8N1, LORATX, LORARX);
  delay(500);
  e220ttl.begin();

#ifdef DEBUG_ENABLE
  //TestErogate();
  testLcd();
  testDHT();
  //testBuz();
  //testUR();
  //e220ttl.resetModule();
  ResponseStructContainer c;
  c = e220ttl.getConfiguration();
  // It's important get configuration pointer before all other operation
  Configuration configuration = *(Configuration*) c.data;
  Serial.println(c.status.getResponseDescription());
  Serial.println(c.status.code);
  
  printParameters(configuration);
  c.close();
  //scanAddress();
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
  // getting the data to send through LoRa

  State status = routine();             // to check if the dispenser is fine
  String _status_ = String(status);
  IRButton button = getButtonIR(); // to check if it has been received an input from the remote
  String _button_ = String(button);
  String msg;

  if(status == ALERT)          //if the Status is on ALERT
  {
    Serial.println("!ALERT!");
    msg = _status_;

    ResponseStatus rs = e220ttl.sendFixedMessage
    (
      TARGET_ADDH,
      TARGET_ADDL,
      TARGET_CHANNEL,
      msg
    )
  }
  else if(status == IDLE)
  {
    switch(button)
    {
      case ZERO: break;
      case ONE: break;
      case TWO: break;
      case THREE: break;
      case FOUR: break;
      case FIVE: break;
      case SIX: break;
      case SEVEN: break;
      case EIGHT: break;
      case NINE: break;
      default: Serial.println("Nothing to send."); return; //if the input received isn't a number
    }

    msg = _status_ + " " + _button_;
    ResponseStatus rs = e220ttl.sendFixedMessage
    (
      TARGET_ADDH,
      TARGET_ADDL,
      TARGET_CHANNEL,
      msg
    )
  }
  //...
  delay(10000);
  return;
}

IRButton getButtonIR() // takes action based on IR code received
{
  IRButton button;
  if(irrecv.decode())
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
      case 0xBA45FF00: button = POWER; break;
      case 0xB847FF00: button = FUNC; break;
      case 0xB946FF00: button = VOLUP; break;
      case 0xBB44FF00: button = FBACK; break;
      case 0xBF40FF00: button = PAUSE; break;
      case 0xBC43FF00: button = FFORW; break;
      case 0xF807FF00: button = DOWN; break;
      case 0xEA15FF00: button = VOLDOWN; break;
      case 0xF609FF00: button = UP; break;
      case 0xE619FF00: button = EQ; break;
      case 0xF20DFF00: button = ST; break;
      case 0xE916FF00: button = ZERO; break;
      case 0xF30CFF00: button = ONE; break;
      case 0xE718FF00: button = TWO; break;
      case 0xA15EFF00: button = THREE; break;
      case 0xF708FF00: button = FOUR; break;
      case 0xE31CFF00: button = FIVE; break;
      case 0xA55AFF00: button = SIX; break;
      case 0xBD42FF00: button = SEVEN; break;
      case 0xAD52FF00: button = EIGHT; break;
      case 0xB54AFF00: button = NINE; break;
      default:
        Serial.println(F("ERROR IRButton"));
        button = ERROR;
    }// End Case
    //store the last decodedRawData
    last_decodedRawData = irrecv.decodedIRData.decodedRawData;
    //delay(500); // Do not get immediate repeat
    irrecv.resume();
  }else
  {
    button = NONE;
  }
  return button;
} //END translateIR

//initial routine at beginning of loop
State routine()
{
  State status = IDLE;
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if(temperature > TEMPMAX || temperature < TEMPMIN)
  {
    status = ALERT;
    //immediately return for emergency
    return status
  }else if (humidity > HUMMAX || humidity < HUMMIN)
  {
    status = ALERT;
    return status;
  }
  if (readMic() > MICMAX)
  {
    status = ALERT;
    return status;
  }
  //TODO erogating

  // to return IDLE if nothing wrong was found
  return status;
}

int readMic()
{
  return analogRead(MICPIN);
}

#ifdef DEBUG_ENABLE
void TestErogate(){
  DEBUG_PRINTLN(F("starting rotating"));
  digitalWrite(DCDIRA, HIGH);
  digitalWrite(DCDIRB, LOW);
  delay(3000);
  DEBUG_PRINTLN(F("stopping rotating"));
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

void testDHT()
{
  // read humidity
  float humi  = dht.readHumidity();
  // read temperature in Celsius
  float tempC = dht.readTemperature();
  DEBUG_PRINT(F("humidity: ")); DEBUG_PRINTLN(humi);
  DEBUG_PRINT(F("temperatue: ")); DEBUG_PRINTLN(tempC);
}

void scanAddress()
{
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }
  delay(5000);
}
#endif