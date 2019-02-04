/*
 * ArduinoJson
 * https://arduinojson.org/v5/example/config/
 * 
 * POST file to SPIFFS
 * https://tttapa.github.io/ESP8266/Chap12%20-%20Uploading%20to%20Server.html
 * 
 */

#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
#define DEBUG_MSG(...)
#endif

#ifndef _Config_h
#define _Config_h

#include "FS.h"           // https://github.com/esp8266/Arduino/tree/master/cores/esp8266
#include "ArduinoJson.h"  // https://arduinojson.org/ https://arduinojson.org/assistant/
#include <pgmspace.h>     // PROGMEM functions
#include <TimeLib.h>      // https://github.com/PaulStoffregen/Time
#include <string.h>       // strncat etc

typedef struct {
  uint8_t hour;
  uint8_t minute;
  // Action function
  // https://stackoverflow.com/questions/1485983/calling-c-class-methods-via-a-function-pointer
  // https://stackoverflow.com/questions/2402579/function-pointer-to-member-function
  // https://stackoverflow.com/questions/18145874/passing-a-pointer-to-a-class-member-function-as-a-parameter
} Alarm;

class Config {
public:
  static const char *attr[];
  static const char config_file[];
  enum ATTR {
    NAME, TEMPMAX, HUMIDMAX, TZ, DST, LEDON, LEDOFF,
    LOGTIME, UPDATETIME, attrNo
  };
                                // Controller name
  char       name[10]      = {'D','e','f','N','a','m','e','\0' }; // Bugfix for gcc 4.9
  uint8_t    humidMax      = 92;     // Humidity High
  float      tempMax       = 28.0;   // Temperature High
  int8_t     tz            = 1;      // TimeZone hours
  bool       dst           = true;   // Daylight Saving Time
  bool       saved         = true;   // Configuration data saved true/false
  time_t     time;                   // Current time
  unsigned long logTime    = 0;      // Milliseconds between log records 0=off
  unsigned long logMillis  = 0;      // Time of next log record in ms
  unsigned long updateTime = 1000;   // Milliseconds between Growbox.update
  unsigned long updMillis  = 0;      // Time to next run of Growbox.update in ms
  
  Alarm      ledOn;                 //
  Alarm      ledOff;                //

  Config();
  ~Config();
  void set( uint8_t d, const char *c );
  bool setAlarm( Alarm &a, const char *c );
  bool setBool( bool &b, const char *c );

  void toJson( char *c, int size );
  void jsonAttribute( char *c, ATTR a, int size );
  void jsonString( char *c, ATTR a, const char* s, int size );
  void jsonInt( char *c, ATTR a, int i, int size );
  void jsonFloat( char *c, ATTR a, float f, int size );
  void load();
  void save();
};
#endif
