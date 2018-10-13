#ifndef _GrowBox_h
#define _GrowBox_h

#define COM
#define I2C
#define INTERVAL 5000

#include <Arduino.h>
#include "FS.h"           // https://github.com/esp8266/Arduino/tree/master/cores/esp8266
#include "Config.h"       // Configuration class for local storage
#include "Clock.h"        // TimeLib with NTP update

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
/* OUTPUT */
#define FAN1     0
#define FAN2     1
#define LED      2
#define AUX      3

#ifdef I2C
#define I2C_OLED  0x3C
#define I2C_DHT12 0x5C
#include <Wire.h>
#include "SSD1306Ascii.h"       // https://github.com/greiman/SSD1306Ascii
#include "SSD1306AsciiWire.h"   // https://github.com/greiman/SSD1306Ascii
#ifdef COM
#define _s Serial
#else
#define _s oled
#endif
#endif

#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h"
}

/* TIME */
#define NTPSERVER       "time.nist.gov"
#define TIMEPORT        8888
#define NTP_PACKET_SIZE 48
#include <TimeLib.h>      // https://github.com/PaulStoffregen/Time
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

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
  time_t   time;
  
  GrowBox();
  void init();
  void update();
  void fetSet( char fetNo, char stat );
  void fetOn( char fetNo );
  void fetOff( char fetNo );
  char fetStatus( char fetNo );
  uint8_t dht12get();
};

time_t  getNtpTime();
bool dst( time_t t, uint8_t tz );

#endif
