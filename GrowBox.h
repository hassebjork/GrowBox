#ifndef _GrowBox_h
#define _GrowBox_h

#include <Arduino.h>
#include <string.h>             // strncat etc
#include "FS.h"                 // https://github.com/esp8266/Arduino/tree/master/cores/esp8266

/* CONTROL */
#define SDA  4
#define SCL  5
#define TX   1
#define RX   3
/* INPUT */
#define IO12 12
#define IO14 14

#define I2C_OLED  0x3C
#define I2C_DHT12 0x5C
#include <Wire.h>
#include "SSD1306Ascii.h"       // https://github.com/greiman/SSD1306Ascii
#include "SSD1306AsciiWire.h"   // https://github.com/greiman/SSD1306Ascii
#include "Config.h"             // Configuration class for local storage
extern Config config;

class GrowBox {
public:
  enum FET {
    LED, FAN, PUMP, AUX, fetNo
  };
  static const char    *fetName[];  // Name of FET pin
  static const uint8_t  fetPin[];   // FET pin number
  static       uint16_t fetValue[]; // Current value
  static       uint16_t fetState[]; // Set value
  static const int      PWM_MAX;
  
  SSD1306AsciiWire oled;
  float    humidity;         // Air humidity
  float    temperature;      // Air temperature
  unsigned long millisUpd;   // Last update of temp/humid

  float    logTemp      = 0.0;
  float    maxTemp      = -300.0;
  float    minTemp      = 400.0;
  float    logHumid     = 0.0;
  float    maxHumid     = 0.0;
  float    minHumid     = 100.0;
  unsigned int logCount = 0;
  
  GrowBox();
  void     update();
  void     logRecord();
  void     toJson( char *c, int size );
  void     fetSet( uint8_t no, uint16_t value );
  uint16_t fetStatus( uint8_t no );
  uint8_t  dht12get( float &t, float &h );

  private:
  void     dim( uint8_t no, int8_t  v=0 );
  void     setValue( uint8_t no, uint16_t value );
};

#endif
