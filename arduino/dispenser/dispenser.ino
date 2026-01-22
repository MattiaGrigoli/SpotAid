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
#include <map>

//IR memory optimizations
#define RAW_BUFFER_LENGHT 50
#define EXCLUDE_EXOTIC_PROTOCOLS
#define DECODE_NEC
#define NO_LED_FEEDBACK_CODE

#include <IRremote.hpp>


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
#define BRIDGE_ADDL 01
#define CHANNEL 0x23
#define DELAYLCD 1500 //1.5 seconds
#define ID_DISPENSER 0 //address LOW 0x02
#define DELAYEROGATION 1000 // 1 second

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

//TODO create a std::map for having a dictionary-like management of the product inside the dispenser
class Product 
{
public:
  int id;
  int count;
  String name;
  float price;

  Product(){
    id = 0;
    count=0;
    name = "";
    price = 0.0;
  };
private:
};

std::map<int, Product> products;

//global variable, also for efficiency
State status;
IRButton buttonIR;
String _status_;
String _button_;
String msg;
unsigned long now; //for display
byte displayIndex;
int buttonState;
bool displayed; // to avoid constant LCD clearing

void setup() {
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

  DEBUG_PRINTLN(F("Initialize button"));
  pinMode(BUTTON, INPUT_PULLUP);

  now = millis();
  displayIndex = 0;
  displayed = false;
  status = ALERT;

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
#ifdef DEBUG_ENABLE

#endif
  // getting the data to send through LoRa

  status = routine();             // to check if the dispenser is fine
  _status_ = String(status);
  buttonIR = getButtonIR(); // to check if it has been received an input from the remote
  _button_ = String(buttonIR);
  buttonState = digitalRead(BUTTON);

  switch(status)
  {
    case ALERT:
    {
      //display "ALERT" on LCD, and check if button was pressed
      if(!displayed)
      {
        displayed = true;
        displayLCD(F("ALERT"), F("out of order!"));
        sendLora(F("ALERT"));
      }
      if(buttonState == HIGH)
      {
        DEBUG_PRINTLN(F("alert button pressed"));
        status = IDLE;
        displayed = false;
      }
      break;
    }
    case EROGATION:
    {
      break;
    }
    case IDLE:
    {
      //if product list is empty
      if(products.size() == 0)
      {
        if(!displayed)
        {
          displayLCD("waiting list", "");
        }else
        {
          break;
        }
      }else
      {
        cycleDisplay();
      }
      if (buttonIR != NONE)
      {
        byte productsNumber = products.size();
        switch(buttonIR)
        {
          case ZERO:
          {
            auto product = std::next(products.begin(), 0);
            if(products[product->second.id].count != 0)
            {
            startErogation();
            DEBUG_PRINT("sending lora selling of "); DEBUG_PRINT(product->second.id);
            sendLora(":"+String(product->second.id));
            products[product->second.id].count--;
            break;
            }else
            {
              DEBUG_PRINTLN("selezionato prodotto finito");
              break;
            }
          }
          case ONE: 
          {
            sendSelling(productsNumber, 1);
            break;
          }
          case TWO:
          {
            sendSelling(productsNumber, 2);
            break;
          }
          case THREE:
          {
            sendSelling(productsNumber, 3);
            break;
          }
          case FOUR:
          {
            sendSelling(productsNumber, 4);
            break;
          }
          case FIVE:
          {
            sendSelling(productsNumber, 5);
            break;
          }
          case SIX:
          {
            sendSelling(productsNumber, 6);
            break;
          }
          case SEVEN:
          {
            sendSelling(productsNumber, 7);
            break;
          }
          case EIGHT:
          {
            sendSelling(productsNumber, 8);
            break;
          }
          case NINE:
          {
            sendSelling(productsNumber, 9);
            break;
          }
          default: break; //if the input received isn't a number
        }
      }
      break;
    }
  }
  //...
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
    } else
    {
      //output the IR code on the serial monitor
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
        DEBUG_PRINTLN(F("ERROR IRButton"));
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
  State state = IDLE;
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if(temperature > TEMPMAX || temperature < TEMPMIN)
  {
    state = ALERT;
    stopErogation();
    //immediately return for emergency
    return state;
  }else if (humidity > HUMMAX || humidity < HUMMIN)
  {
    state = ALERT;
    stopErogation();
    return state;
  }
  if (readMic() > MICMAX)
  {
    state = ALERT;
    stopErogation();
    return state;
  }
  if(status == EROGATION)
  {
    if(millis() > now + DELAYEROGATION)
    {
      stopErogation();
      state = IDLE;
    }
  }

  // to return IDLE if nothing wrong was found
  return state;
}

int readMic()
{
  return analogRead(MICPIN);
}

ResponseStatus sendLora(String msg)
{
  ResponseStatus rs = e220ttl.sendFixedMessage
  (
    0,
    BRIDGE_ADDL,
    CHANNEL,
    String(ID_DISPENSER) + msg  //to identify always the dispener
  );
  return rs;
}

//TO TEST
void listenLora()
{
  if (e220ttl.available() > 0) 
  {
    products.clear();
    ResponseContainer msg = e220ttl.receiveMessage();
    String input = msg.data;

    DEBUG_PRINT("Received: ");
    DEBUG_PRINTLN(msg.data);

    int startIndex = 0;
    int endIndex = input.indexOf(';');

    while (startIndex < input.length()) {
        if (endIndex == -1) endIndex = input.length();
        
        // Estraiamo la stringa del singolo prodotto: "1,Pasta,1.50,5"
        String productStr = input.substring(startIndex, endIndex);
        
        // Ora dividiamo i campi (separatore ',')
        int c1 = productStr.indexOf(',');
        int c2 = productStr.indexOf(',', c1 + 1);
        int c3 = productStr.indexOf(',', c2 + 1);

        if (c1 != -1 && c2 != -1 && c3 != -1) {
            int id = productStr.substring(0, c1).toInt();
            String name = productStr.substring(c1 + 1, c2);
            float price = productStr.substring(c2 + 1, c3).toFloat();
            int qty = productStr.substring(c3 + 1).toInt();

            //Serial.println("Ricevuto -> ID: %d, Nome: %s, Prezzo: %.2f, Qta: %d\n", id, name.c_str(), price, qty);
            DEBUG_PRINT("Ricevuto -> ID: ");
            DEBUG_PRINT(id);
            DEBUG_PRINT(", Nome: ");
            DEBUG_PRINT(name.c_str());
            DEBUG_PRINT(", Prezzo: ");
            DEBUG_PRINT(price);
            DEBUG_PRINT(", Qta: ");
            DEBUG_PRINT("qty");

            Product product;
            product.id = id;
            product.price = price;
            product.count = qty;
            product.name = name;
            products.insert({product.id, product});
        }

        startIndex = endIndex + 1;
        endIndex = input.indexOf(';', startIndex);
    }
  }
}

void cycleDisplay()
{
  if(millis() > now + DELAYLCD)
  {
    now = millis();
    if(displayIndex == products.size()-1)
    {
      displayIndex = 0;
    }else
    {
      displayIndex++;
    }
    auto product = std::next(products.begin(), displayIndex);
    if(products[product->second.id].count != 0)
    {
      String name= product->second.name;
      if(name.length() > 11)
      {
        name = name.substring(0,11);
      }
      displayLCD("Select a product", String(displayIndex) + ":" + name + " " + String(product->second.price, 2));
    }else
    {
      return;
    }
  }else
  {
    return;
  }
}

void displayLCD(String msgTop, String msgUnder)
{
  if(msgTop.length() > 16 || msgUnder.length() > 16)
  {
    return;
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(msgTop);
  lcd.setCursor(0,1);
  lcd.print(msgUnder);
  return;
}

void startErogation()
{
  status = EROGATION;
  displayLCD(F("Erogating..."), F("Please wait"));
  now = millis();
  DEBUG_PRINTLN(F("starting rotating"));
  digitalWrite(DCDIRA, HIGH);
  digitalWrite(DCDIRB, LOW);
  return;
}

void stopErogation()
{
  DEBUG_PRINTLN(F("stopping rotating"));
  digitalWrite(DCDIRA, LOW);
  digitalWrite(DCDIRB, LOW);
  return;
}

void sendSelling(byte limit, byte index)
{
  if(limit > index)
  {
    auto product = std::next(products.begin(), index);
    if(products[product->second.id].count != 0)
    {
      startErogation();
      DEBUG_PRINT("sending lora selling of "); DEBUG_PRINT(product->second.id);
      sendLora(":"+String(product->second.id));
      products[product->second.id].count--;
      return;
    }else
    {
      DEBUG_PRINTLN("selezionato prodotto finito");
      return;
    }
  }else
  {
    DEBUG_PRINTLN("not in index");
    return;
  }
}

#ifdef DEBUG_ENABLE
//TO FIX
void TestErogate(){
  DEBUG_PRINTLN(F("starting rotating"));
  digitalWrite(DCDIRA, HIGH);
  digitalWrite(DCDIRB, LOW);
  delay(3000);
  digitalWrite(DCDIRA, LOW);
  digitalWrite(DCDIRB, LOW);
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