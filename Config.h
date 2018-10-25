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
  uint8_t    id;          // id of controller
  char       name[10];    // Controller name
  char       ssid[32];    // Network SSID
  char       pass[64];    // Network password
  uint8_t    humidMin;    // Humidity Low
  uint8_t    humidMax;    // Humidity High
  float      tempMin;     // Temperature Low
  float      tempMax;     // Temperature High
  int8_t     tz;          // TimeZone
  uint8_t    dst;         // Daylight Saving Time
  
  Config() {
  }
  
//  void save( const char *filename ) {
//    SPIFFS.begin();
//    SPIFFS.remove( filename );
//    File file = SPIFFS.open( filename, "W" );
//    if ( !file )
//      return;
//    file.write( (uint8_t*)this, sizeof( Config ) );
//    file.close();
//  }
//  
//  void load( const char *filename ) {
//    SPIFFS.begin();
//    File file = SPIFFS.open( filename, "R" );
//    if ( !file )
//      return;
//    file.read( (uint8_t*)this, sizeof( Config ) );
//    file.close();
//  }
  
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
      humidMin = root["humidity"]["min"] | 30;
      humidMax = root["humidity"]["max"] | 95;
      tempMin  = root["temperature"]["min"] | 14.0;
      tempMax  = root["temperature"]["max"] | 28.0;
      tz       = root["time"]["tz"]  | 1;
      dst      = root["time"]["dst"] | 1;
      strlcpy( ssid, root["network"][0]["ssid"] | "", sizeof( ssid ) );
      strlcpy( pass, root["network"][0]["password"] | "", sizeof( pass ) );
    }
    f.close();
  }
  
  void toJson( char *c, int size ) {
    char buff[10];
    strncpy( c, "{\"id\":", size ); 
    itoa( id, buff, 10 );
    strncat( c, buff, size ); 
    strncat( c, ",\"name\":\"", size ); 
    strncat( c, name, size ); 
    strncat( c, "\",\"humid\":[", size ); 
    itoa( humidMin, buff, 10 );
    strncat( c, buff, size ); 
    strncat( c, ",", size ); 
    itoa( humidMax, buff, 10 );
    strncat( c, buff, size ); 
    strncat( c, "],\"temp\":[", size ); 
    dtostrf( tempMin, 0, 1, buff );
    strncat( c, buff, size ); 
    strncat( c, ",", size ); 
    dtostrf( tempMax, 0, 1, buff );
    strncat( c, buff, size ); 
    strncat( c, "],\"tz\":", size ); 
    itoa( tz, buff, 10 );
    strncat( c, buff, size ); 
    strncat( c, ",\"dst\":", size ); 
    itoa( dst, buff, 10 );
    strncat( c, buff, size );
    strncat( c, ",\"wifi\":[\"", size ); 
    strncat( c, ssid, size ); 
    strncat( c, "\",\"", size ); 
    strncat( c, pass, size ); 
    strncat( c, "\"]", size ); 
    strncat( c, "}", size ); 
  }
  
  void write( const char *filename ) {
    SPIFFS.begin();
    SPIFFS.remove( filename );
    File file = SPIFFS.open( filename, "W" );
    if ( !file ) {
      Serial.println( F( "Failed to create file" ) );
      return;
    }
    char temp[500];
    snprintf_P( temp, sizeof( temp ),
PSTR("{\"id\":%d,\"name\":\"%s\",\
\"humidity\":{\"min\":%d,\"max\":%d},\
\"temperature\":{\"min\":%f,\"max\":%f},\
\"tz\":%d,\"dst\":%d,\
}"),
    id, name, humidMin, humidMax, tempMin, tempMax );
    
    file.print( temp );
    file.close();
  }
};
#endif
