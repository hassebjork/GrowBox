#include "Config.h"

const char *Config::attr[] = { 
    "tempMax", "humidMax", "tz", "dst", "ledOn", "ledOff" 
};

Config::Config() {
  set( LEDON,  "2400" );
  set( LEDOFF, "2400" );
  read();
}
  
void Config::set( uint8_t d, const char *c ) {
  int i;
  char buff[65];
  DEBUG_MSG("Config::set( %d, '%s' )\n", d, c );  
  
  switch( d ) {
    case TEMPMAX:
      i = atof( c );
      if ( i > 0.0 && i < 50.0 ) {
        saved = ( i == tempMax );
        tempMax = i;
      }
      break;
    case HUMIDMAX:
      i = atoi( c );
      if ( i >= 0 && i <= 100 ) {
        saved = ( i == humidMax );
        humidMax = i;
      }
      break;
    case TZ:
      i = atoi( c );
      if ( i > -13 && i < 13 ) {
        saved = ( i == tz );
        tz = i;
        saved = false;
      }
      break;
    case DST:
      i = atoi( c );
      if ( i == 0 || i == 1 ) {
        saved = ( i == dst );
        dst = i;
        saved = false;
      }
      break;
    case LEDON:
      i = atoi( c );
      if ( i >= 0 && i <= 2359 ) {
        ledOn.hour    = i / 100;
        ledOn.minute  = i % 100;
        saved = false;
      } else {
        ledOn.hour    = 25;
        ledOn.minute  = 00;
      }
      DEBUG_MSG( "LEDON: %s %d=%d:%d\n", c, i, ledOn.hour, ledOn.minute );
      break;
    case LEDOFF:
      i = atoi( c );
      if ( i >= 0 && i <= 2359 ) {
        ledOff.hour    = i / 100;
        ledOff.minute  = i % 100;
        saved = false;
      } else {
        ledOff.hour    = 25;
        ledOff.minute  = 00;
      }
      DEBUG_MSG( "LEDOFF: %s %d=%d:%d\n", c, i, ledOff.hour, ledOff.minute );
      break;
    default:
      saved = true;
  }
}
  
void Config::toJson( char *c, int size ) {
  char buff[10];
  strncpy( c, "{\"name\":\"", size ); 
  strncat( c, name, size ); 
  strncat( c, "\",\"humidMax\":", size ); 
  itoa( humidMax, buff, 10 );
  strncat( c, buff, size ); 
  strncat( c, ",\"tempMax\":", size ); 
  dtostrf( tempMax, 0, 1, buff );
  strncat( c, buff, size ); 
  strncat( c, ",\"tz\":", size );
  itoa( tz, buff, 10 );
  strncat( c, buff, size ); 
  strncat( c, ",\"dst\":", size ); 
  itoa( dst, buff, 10 );
  strncat( c, buff, size );
  strncat( c, ",\"ledOn\":", size );
  itoa( (uint16_t) (ledOn.hour * 100)  + ledOn.minute,  buff, 10 );
  strncat( c, buff, size );
  strncat( c, ",\"ledOff\":", size ); 
  itoa( (uint16_t) (ledOff.hour * 100) + ledOff.minute, buff, 10 );
  strncat( c, buff, size );
  strncat( c, "}", size );
//  DEBUG_MSG("Config::toJson: %d bytes\n", strlen( c ) );  
}
  
void Config::read() {
  SPIFFS.begin();
  File f = SPIFFS.open( FILE_CONFIG, "r" );
  if ( !f ) {
    Serial.println( String( "Error opening " ) + String( FILE_CONFIG ) );
  } else {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson( doc, f );
    if (error)
      Serial.println( String("Default configuration: ") );
    JsonObject root = doc.as<JsonObject>();
    int i;
    char c[6];
    strlcpy( name, root["name"] | "DefName", sizeof( name ) );
    set( HUMIDMAX, root["humidMax"] | "92" );
    set( TEMPMAX,  root["tempMax"]  | "28.0" );
    set( TZ,       root["tz"]       | "1" );
    set( DST,      root["dst"]      | "1" );
//    strlcpy( c, root["ledOn" ], 5 );
//    set( LEDON,    c );
//    c = root["ledOff"];
//    set( LEDOFF,   c );
  }
  f.close();
  saved = true;
}
  
void Config::write() {
  SPIFFS.begin();
  SPIFFS.rename( FILE_CONFIG, String( FILE_CONFIG ) + String( ".old" ) );
  File file = SPIFFS.open( FILE_CONFIG, "W" );
  if ( !file ) {
    Serial.println( F( "Failed to create file" ) );
    return;
  }
  char temp[200];
  toJson( temp, 200 );
  file.print( temp );
  file.close();
  saved = true;
}
