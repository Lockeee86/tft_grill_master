/***************************************************
  Grill-Thermometer
  ...wenn Männer kochen
  Equitment:
  - Adafruit Feather
  - Adafruit TFT Feather-Wing
  - 4 Analog NTC (Fleisch und Grill Thermometer)
  - Akku Pack 2000mA

  Weber Thermometer Value
    float T0_temp_fleisch=24;    // Nenntemperatur des NTC-Widerstands in °C
    float R0_temp_fleisch=240000; // Nennwiderstand des NTC-Sensors in Ohm
    float T1_temp_fleisch=100;   // erhöhte Temperatur des NTC-Widerstands in °C
    float R1_temp_fleisch=13100;  // Widerstand des NTC-Sensors bei erhöhter Temperatur in Ohm
  IKEA Fantast 2.
    float T0_temp_fleisch=27;    // Nenntemperatur des NTC-Widerstands in °C
    float R0_temp_fleisch=214000; // Nennwiderstand des NTC-Sensors in Ohm
    float T1_temp_fleisch=100;   // erhöhte Temperatur des NTC-Widerstands in °C
    float R1_temp_fleisch=13000;  // Widerstand des NTC-Sensors bei erhöhter Temperatur in Ohm
  
 ****************************************************/
#include "config.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <math.h>


/*-------------------------- TFT Display --------------------------------------------*/
#ifdef ARDUINO_SAMD_FEATHER_M0
   #define STMPE_CS 6
   #define TFT_CS   9
   #define TFT_DC   10
   #define SD_CS    5
#endif

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

/*-------------------------- Parameter ----------------------------------------------*/
// define analog value
#define Grill A2
#define Grill2 A0
#define Fleisch A3
#define Fleisch2 A1
#define ABSZERO 273.15
#define MAXANALOGREAD 1023.0

// value timer
unsigned long previousMillis = 0;
unsigned long interval =   10000;

bool con_ada LOW;

// tx for data string
char TX[20];

// berechnung b, abs, und rn
float temperature_NTC(float T0, float R0, float T1, float R1, float RV, float VA_VB)
{
 T0+=ABSZERO;  // umwandeln Celsius in absolute Temperatur
 T1+=ABSZERO;  // umwandeln Celsius in absolute Temperatur
 float B= (T0 * T1)/ (T1-T0) * log(R0/R1); // Materialkonstante B
 float RN=RV*VA_VB / (1-VA_VB); // aktueller Widerstand des NTC
 return T0 * B / (B + T0 * log(RN / R0))-ABSZERO;
}

// IKEA Fantast einstellungen
float T0=27;    // Nenntemperatur des NTC-Widerstands in °C
float R0=214000; // Nennwiderstand des NTC-Sensors in Ohm
float T1=100;   // erhöhte Temperatur des NTC-Widerstands in °C
float R1=13000;  // Widerstand des NTC-Sensors bei erhöhter Temperatur in Ohm
float Vorwiderstand=10000; // Vorwiderstand in Ohm  

float temp_grill;
float temp_grill2;
float temp_fleisch;
float temp_fleisch2;

/*-------------------------- Adafruit IO --------------------------------------------*/
AdafruitIO_Feed *io_grill1 = io.feed("grill");
AdafruitIO_Feed *io_fleisch1 = io.feed("fleisch");
AdafruitIO_Feed *io_grill2 = io.feed("grill2");
AdafruitIO_Feed *io_fleisch2 = io.feed("fleisch2");

/*-------------------------- function -----------------------------------------------*/
void logo();
void data_screen();

/*-------------------------- SETUP function -----------------------------------------*/
void setup() {
  Serial.begin(115200);
  tft.begin();
  //show logo
  logo();
 
  tft.setCursor(0, 70);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); tft.setTextSize(1);
  tft.println("Connecting to Adafruit IO");
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // wait for a connection
  int i =0;
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    tft.print(".");
    if(i==24){
      break;
    }
    i++;
    delay(500);
  }
  // connect status
  Serial.println();
  Serial.println(io.statusText());
  tft.println();
  tft.println(io.statusText());
  
  //reprint logo
  logo();
  delay(3000);
}

/*-------------------------- LOOP function -------------------------------------------*/
void loop() {
  //Funktioniert nur alleine mit io.run();
  io.run();
  data_screen();   
  delay(10);
}

/*-------------------------- data_screen() ----------------------------------------------*/
void data_screen(){
  
  // ---------------------- Analog Value Temp ---------------------------------------------
  int aValue0=analogRead(Grill);
  int aValue1=analogRead(Fleisch);
  int aValue3=analogRead(Grill2);
  int aValue2=analogRead(Fleisch2);

  // ---------- Berechnen bei unbekannter Materialkonstante für Grill und Fleisch ---------
  temp_grill=temperature_NTC(T0, R0, T1, R1, Vorwiderstand, aValue0/MAXANALOGREAD);  
  temp_fleisch=temperature_NTC(T0, R0, T1, R1, Vorwiderstand, aValue1/MAXANALOGREAD);
  temp_grill2=temperature_NTC(T0, R0, T1, R1, Vorwiderstand, aValue3/MAXANALOGREAD);
  temp_fleisch2=temperature_NTC(T0, R0, T1, R1, Vorwiderstand, aValue2/MAXANALOGREAD);
 
  int data_grill_temp = roundf(temp_grill);
  int data_grill_temp2 = roundf(temp_grill2);
  int data_temp_fleisch = roundf(temp_fleisch);
  int data_temp_fleisch2 = roundf(temp_fleisch2);

  //Grill 1 60 - 110 px --------------------------------------------
  tft.drawRect(0, 60, 240, 110, ILI9341_WHITE);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK); tft.setTextSize(1); 
  tft.setCursor(5, 65); tft.println("Grill 1:");
  
  // Grill 1 Temp
  tft.setTextSize(2);
  if (data_grill_temp == -273){
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK); 
    sprintf(TX,"Grill   :   N/A ");  
  } else {
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
    sprintf(TX,"Grill   : %4d C", data_grill_temp);
  }
  tft.setCursor(40, 90);  tft.print(TX);
  
  //Fleisch 1 Temp
  if (data_temp_fleisch == -273){
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK); 
    sprintf(TX,"Fleisch :   N/A ");  
  } else {
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
    sprintf(TX,"Fleisch : %4d C", data_temp_fleisch);
  }
  tft.setCursor(40, 120);  tft.print(TX);
  
  // Grill 2 110 - 180 ---------------------------------------------
  tft.drawRect(0, 180, 240, 110, ILI9341_WHITE); 
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK); tft.setTextSize(1); 
  tft.setCursor(5, 185); tft.println("Grill 2:");

  // Grill 2 Temp
  tft.setTextSize(2); 
  if (data_grill_temp2 == -273){
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK); 
    sprintf(TX,"Grill   :   N/A ");  
  } else {
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
    sprintf(TX,"Grill   : %4d C", data_grill_temp2);
  }
  tft.setCursor(40, 210);  tft.print(TX);
  
  //Fleisch 2 Temp
  if (data_temp_fleisch2 == -273){
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK); 
    sprintf(TX,"Fleisch :   N/A ");  
  } else {
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); 
    sprintf(TX,"Fleisch : %4d C", data_temp_fleisch2);
  }
  tft.setCursor(40, 240);  tft.print(TX);
    // Send to adafruit io
    if (millis() - previousMillis > interval) {
        if (data_grill_temp == -273){
          io_grill1->save(0);
        } else {
          io_grill1->save(data_grill_temp);
        }
        delay(10);
        if (data_temp_fleisch == -273){
          io_fleisch1->save(0);
        } else {
          io_fleisch1->save(data_temp_fleisch);
        }
        delay(10);
        if (data_grill_temp2 == -273){
          io_grill2->save(0);
        } else {
          io_grill2->save(data_grill_temp2);
        }
        delay(10);
        if (data_temp_fleisch2 == -273){
          io_fleisch2->save(0);
        } else {
          io_fleisch2->save(data_temp_fleisch2);
        }
        delay(10);
        previousMillis = millis();
        Serial.println("--Data to AIO--");
        Serial.print("Grill-Temp:    ");
        Serial.print(data_grill_temp);
        Serial.println(" °C");
        Serial.print("Fleisch-Temp:  ");
        Serial.print(data_temp_fleisch);
        Serial.println(" °C");
        Serial.print("Grill2-Temp:   ");
        Serial.print(data_grill_temp2);
        Serial.println(" °C");
        Serial.print("Fleisch2-Temp: ");
        Serial.print(data_temp_fleisch2);
        Serial.println(" °C");
        Serial.println(" ");
    }
}

/*-------------------------- Logo() ----------------------------------------------*/
void logo() {
  tft.setRotation(0); tft.fillScreen(ILI9341_BLACK); 
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK); tft.setTextSize(3); 
  tft.setCursor(0, 5); tft.println("Grillen"); 
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); tft.setTextSize(2);
  tft.print("wenn Maenner kochen");
  tft.drawLine(0, 50, 240, 50, ILI9341_RED);
  
  // Show connection Status
  tft.drawLine(0, 300, 240, 300, ILI9341_RED);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); tft.setTextSize(1);
  tft.setCursor(0, 310); tft.println(io.statusText());
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); tft.setTextSize(2);
}
