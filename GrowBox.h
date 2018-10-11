#ifndef _GrowBox_h
#define _GrowBox_h

#include <Arduino.h>

#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

#define COM
#define I2C

#define INTERVAL 1000
/* CONTROL */
#define SDA  4
#define SCL  5
#define TX   1
#define RX   3
/* INPUT */
#define IO12 12
#define IO14 14
/* OUTPUT */
#define FAN1     0
#define FAN2     1
#define LED      2
#define AUX      3

#ifdef I2C
#define I2C_OLED  0x3C
#define I2C_DHT12 0x5C
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#endif

typedef struct {
  uint8_t id;           // id of controller
  uint8_t minHumid;     // Humidity Low
  uint8_t maxHumid;     // Humidity High
  float   minTemp;      // Temperature Low
  float   maxTemp;      // Temperature High
} Config;

typedef struct {
  uint8_t fetState;     // Last state of FETs
  float   humidity;     // Air humidity
  float   temperature;  // Air temperature
  unsigned long previousMillis;
} Data;

class GrowBox {
public:
  Config   config;
  Data     data;
  SSD1306AsciiWire oled;
  float    logTemp;
  float    logHumid;
  uint16_t logCount;
  
  GrowBox();
  void init();
  void update();
  void fetSet( char fetNo, char stat );
  void fetOn( char fetNo );
  void fetOff( char fetNo );
  char fetStatus( char fetNo );
  
  uint8_t dht12get();
};

#endif
