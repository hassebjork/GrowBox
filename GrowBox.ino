#include "GrowBox.h"
GrowBox growBox;

#define COM
#define I2C

/* INPUT */
#define IO12 12
#define IO14 14

/* CONTROL */
#define SDA  4
#define SCL  5
#define TX   1
#define RX   3

#ifdef I2C
#define I2C_OLED  0x3C
#define I2C_DHT12 0x5C
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
SSD1306AsciiWire oled;
#endif

unsigned long previousMillis = 0;
const long interval = 1000;

void setup(void){
#ifdef COM
  Serial.begin( 115200 );
  Serial.println();
  Serial.println("Booting Sketch...");
#else
  pinMode( TX,  OUTPUT );
  pinMode( RX,  OUTPUT );
#endif
  
#ifdef I2C
  Wire.begin( SDA, SCL );
  oled.begin( &Adafruit128x64, I2C_OLED );
  oled.set400kHz();  
  oled.setFont(Adafruit5x7);  
#else
  pinMode( SDA, OUTPUT );
  pinMode( SCL, OUTPUT );
#endif
  
  pinMode( IO12, INPUT );
  pinMode( IO14, INPUT );
  
}

void loop(void){
  
  unsigned long currentMillis = millis();
  int sec = currentMillis / 1000;
  int min = sec / 60;
  int hr = min / 60;
  uint8_t temp;
  
 if ( currentMillis - previousMillis >= interval ) {
    previousMillis += interval;
#ifdef COM
    Serial.print("Time: ");
    Serial.print( (int) hr );
    Serial.print(":");
    Serial.print( (int) min % 60 );
    Serial.print(":");
    Serial.println( (int) sec % 60 );
 #endif
#ifdef I2C
    oled.home();
    oled.print("Time:");
    oled.setCursor( 40, 0 );
    oled.print( (int) ( currentMillis / 3600000 ) );
    oled.print(":");
    oled.print( (int) ( currentMillis / 60000 ) % 60 );
    oled.print(":");
    oled.print( (int) ( currentMillis / 1000 ) % 60 );
    oled.clearToEOL();

    temp = growBox.dht12get( I2C_DHT12 );
    if ( temp == 0 ) {
    oled.setCursor( 0, 1 );
    oled.print( "Temp: " );
    oled.setCursor( 40, 1 );
    oled.print( growBox.config.temperature );
    oled.clearToEOL();
    oled.setCursor( 0, 2 );
    oled.print( "Humid: " );
    oled.setCursor( 40, 2 );
    oled.print( growBox.config.humidity );
    oled.clearToEOL();
    } else {
      oled.print( "DHT12 error " );
      oled.println( temp );
    }
    
 #endif
 
  }
  
}
