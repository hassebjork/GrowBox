/*
 * ArduinoJson
 * https://arduinojson.org/v5/example/config/
 * 
 * Binary Files
 * https://stackoverflow.com/questions/5506645/how-to-read-write-a-struct-in-binary-files
 * https://stackoverflow.com/questions/11928053/read-write-a-complex-struct-into-the-binary-file
 * https://stackoverflow.com/a/46948683/4405465
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
  static const char *months[];
  static const char config_file[];
  enum ATTR {
    NAME, TEMPMAX, HUMIDMAX, TZ,
	DST, LEDON, LEDOFF, LOGTIME,
	UPDATETIME, DIMSTEP, attrNo		// Only json-file change
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
  uint8_t    dimStep       = 10;     // Dimmer steps
  
  Alarm      ledOn;                 //
  Alarm      ledOff;                //

  Config();
  ~Config();
  void set( uint8_t d, const char *c );
  bool setAlarm( Alarm &a, const char *c );
  bool setBool( bool &b, const char *c );
  
  time_t today( uint8_t hr, uint8_t min, uint8_t sec );
  time_t today( uint16_t hrmin );
  bool checkDst();
  void timeRefresh();
  
  void toJson( char *c, int size );
  static void jsonAttribute( char *c, const char *a, bool k, int size );
  void jsonAttribute( char *c, ATTR a, int size );
  static void toJson( char *c, const char* s, int size );
  void toJson( char *c, ATTR a, const char* s, int size );
  static void toJson( char *c, int i, int size );
  void toJson( char *c, ATTR a, int i, int size );
  static void toJson( char *c, float f, int size );
  void toJson( char *c, ATTR a, float f, int size );
  void load();
  void save();
};
#endif
