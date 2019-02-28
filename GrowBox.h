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
#define I2C_AM2320 0xB8

#include <Wire.h>
#include "SSD1306Ascii.h"       // https://github.com/greiman/SSD1306Ascii
#include "SSD1306AsciiWire.h"   // https://github.com/greiman/SSD1306Ascii
#include "Config.h"             // Configuration class for local storage
#include "Log.h"                // Log temperature & humidity
extern Config config;

class GrowBox {
public:
  enum FET {
    LED, FAN, PUMP, AUX, fetNo
  };
  enum ERROR {
    NO_ERROR, ERROR_CONNECT, ERROR_TIMEOUT, ERROR_CHECKSUM
  };
  static const char    *fetName[];  // Name of FET pin
  static const uint8_t  fetPin[];   // FET pin number
  static       uint16_t fetValue[]; // Current value
  static       uint16_t fetState[]; // Set value
  static const int      PWM_MAX;
  static const char     TEMP[];  	// "temp"
  static const char     HUMID[];  	// "humid"
  
  SSD1306AsciiWire oled;
  float    humidity;         // Air humidity
  float    temperature;      // Air temperature
  unsigned long millisUpd;   // Last update of temp/humid
  Log      log;
  
  GrowBox();
  void     update();
  void     toJson( char *c, int size );
  void     fetSet( uint8_t no, uint16_t value );
  uint16_t fetStatus( uint8_t no );
  uint8_t  dht12get( float &t, float &h );
#ifdef AM2320
  uint8_t  am2320get( float &t, float &h );
  uint16_t crc16( uint8_t *buf, uint8_t no );
#endif
  private:
  void     dim( uint8_t no, int8_t  v=0 );
  void     setValue( uint8_t no, uint16_t value );
};

#endif
