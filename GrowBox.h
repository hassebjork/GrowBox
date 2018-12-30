#ifndef _GrowBox_h
#define _GrowBox_h

#define COM
#define INTERVAL_UPD  5000  // Update sensor interval (ms)
#define INTERVAL_CALC 5000  // Calculate average

#include <Arduino.h>
#include "FS.h"           // https://github.com/esp8266/Arduino/tree/master/cores/esp8266

#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

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
    LED, FAN1, FAN2, AUX, fetNo
  };
  static const char    *fetName[];
  static const uint8_t  fetPin[];
  static       uint16_t fetState[];
  static const int      PWM_MAX;
  
  SSD1306AsciiWire oled;
  float    humidity;         // Air humidity
  float    temperature;      // Air temperature
  unsigned long millisUpd;   // Last update of temp/humid
  unsigned long logMillis;   // Last avg calculation
  float    logTemp;
  float    logHumid;
  uint16_t logCount;
  
  GrowBox();
  void     init();
  void     update();
  void     doActivate();
  void     toJson( char *c, int size );
  void     fetSet( uint8_t no, uint16_t value );
  uint16_t fetStatus( uint8_t no );
  uint8_t  dht12get( float &t, float &h );

  private:
  void     dim( uint8_t no, int8_t  v=0 );
};

#endif
