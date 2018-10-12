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
 * 
 */

#ifndef _Config_h
#define _Config_h

#include "FS.h"           // https://github.com/esp8266/Arduino/tree/master/cores/esp8266
#include <ArduinoJson.h>  // https://arduinojson.org/
                          // https://arduinojson.org/assistant/

class Config {
public:
  uint8_t  id;          // id of controller
  char     name[10];    // Controller name
  uint8_t  minHumid;    // Humidity Low
  uint8_t  maxHumid;    // Humidity High
  float    minTemp;     // Temperature Low
  float    maxTemp;     // Temperature High
  
  
  Config() {
  }
  
  void read( const char *filename ) {
    SPIFFS.begin();
    File f = SPIFFS.open( filename, "r" );
    if ( !f ) {
      Serial.println( String( "Error opening " ) + String( filename ) );
    } else {
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson( doc, f );
      if (error)
        Serial.println( String("Default configuration: ") + error );
      JsonObject root = doc.as<JsonObject>();
      
      strlcpy( name, root["name"] | "DefName", sizeof( name ) );
      id       = root["id"] | 0;
      minHumid = root["humidity"]["min"] | 30;
      maxHumid = root["humidity"]["max"] | 95;
      minTemp  = root["temperature"]["min"]  | 14.0;
      maxTemp  = root["temperature"]["max"]  | 28.0;
    }
    f.close();
  }
  
  void write( const char *filename ) {
    SPIFFS.begin();
    SPIFFS.remove( filename );
    File file = SPIFFS.open( filename, "W" );
    if ( !file ) {
      Serial.println( F( "Failed to create file" ) );
      return;
    }

    file.print( "{" );
    file.print( String( "\"id\":"     ) + String( id       ) +String( "," ) );
    file.print( String( "\"name\":\"" ) + String( name     ) +String( "\"," ) );
    file.print( String( "\"humidity\":{\"min\":"    ) + minHumid + String( ",\"max\"" ) + maxHumid + String( "}," ) );
    file.print( String( "\"temperature\":{\"min\":" ) + minTemp  + String( ",\"max\"" ) + maxTemp  + String( "}," ) );
    file.print( "}" );
    file.close();
  }
};
#endif
