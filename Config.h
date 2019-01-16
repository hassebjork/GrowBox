/*
 * Upload file
 * https://github.com/G6EJD/ESP32-8266-File-Download/blob/master/ESP_File_Download_v01.ino
 * 
 * Filesystem calls
 * http://esp8266.github.io/Arduino/versions/2.0.0/doc/filesystem.html
 * 
 * ArduinoJson
 * https://arduinojson.org/v5/example/config/
 * 
 * SPIFFS documentation
 * https://circuits4you.com/2018/01/31/example-of-esp8266-flash-file-system-spiffs/
 * http://arduino.esp8266.com/versions/1.6.5-1044-g170995a/doc/reference.html#file-system
 * 
 * SPIFFS Example
 * https://www.esp8266.com/viewtopic.php?f=29&t=8194
 * https://blog.squix.org/2015/08/esp8266arduino-playing-around-with.html
 * 
 * POST file to SPIFFS
 * https://tttapa.github.io/ESP8266/Chap12%20-%20Uploading%20to%20Server.html
 * 
 * WiFi Manager
 * https://github.com/tzapu/WiFiManager
 */

#ifdef DEBUG_ESP_PORT
#define DEBUG_MSG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
#define DEBUG_MSG(...)
#endif

#ifndef _Config_h
#define _Config_h

#include "FS.h"           // https://github.com/esp8266/Arduino/tree/master/cores/esp8266
#include <ArduinoJson.h>  // https://arduinojson.org/
                          // https://arduinojson.org/assistant/

#define FILE_CONFIG "/config.json"

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
  enum ATTR {
    TEMPMAX, HUMIDMAX, SSID, PASS, TZ, DST, LEDON, LEDOFF, attrNo
  };

  char       name[10];    // Controller name
  char       ssid[33];    // Network SSID
  char       pass[65];    // Network password
  uint8_t    humidMax;    // Humidity High
  float      tempMax;     // Temperature High
  uint8_t    tz;          // TimeZone hours
  uint8_t    dst;         // Daylight Saving Time
  bool       saved;       // Configuration data saved true/false
  
  Alarm      ledOn;       //
  Alarm      ledOff;      //

  Config();
  void set( uint8_t d, const char *c );
  void toJson( char *c, int size );
  void read();
  void write();  
};
#endif
