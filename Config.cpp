#include "Config.h"

const char *Config::attr[] = { 
    "tempMax", "humidMax", "tz", "dst", "ledOn", "ledOff" 
};

const char Config::config_file[] PROGMEM = "/growbox.json";

Config::Config() {
  ledOn.hour    = 26;
  ledOn.minute  = 60;
  ledOff.hour   = 26;
  ledOff.minute = 60;
  load();
}
  
void Config::set( uint8_t d, const char *c ) {
  uint8_t i;
  DEBUG_MSG("Config::set( %d, '%s' )\n", d, c );  
  
  switch( d ) {
    case TEMPMAX:
      float f;
      f = atof( c );
      if ( f > 0.0 && f < 50.0 ) {
        saved = ( f == tempMax );
        tempMax = f;
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
      }
      break;
    case DST:
      i = atoi( c );
      if ( i == 0 || i == 1 ) {
        saved = ( i == dst );
        dst = i;
      }
      break;
    case LEDON:
      saved = setAlarm( ledOn, c );
      break;
    case LEDOFF:
      saved = setAlarm( ledOff, c );
      break;
  }
}

bool Config::setAlarm( Alarm &a, const char *c ) {
  uint8_t hour   = a.hour;
  uint8_t minute = a.minute;
  int    i = atoi( c );
  if ( i >= 0 && i <= 2359 ) {
    a.hour    = i / 100;
    a.minute  = i % 100;
  } else {
    a.hour    = 25;
    a.minute  = 00;
  }
  return ( hour == a.hour && minute == a.minute );
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
  
void Config::load() {
  DEBUG_MSG("Config::load()\n" );  
  SPIFFS.begin();
  File f = SPIFFS.open( FPSTR( config_file ), "r" );
  if ( !f ) {
    DEBUG_MSG("Config::load error: No file!\n" );  
    return;
  }
  char c[6];
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson( doc, f );
  if (error) {
    DEBUG_MSG("Config::load error: Deserialization failed!\n" );  
  } else {
    JsonObject root = doc.as<JsonObject>();
    strlcpy( name, root["name"] | "DefName", sizeof( name ) );
    set( HUMIDMAX, root["humidMax"] | "92" );
    set( TEMPMAX,  root["tempMax"]  | "28.0" );
    set( TZ,       root["tz"]       | "1" );
    set( DST,      root["dst"]      | "1" );
    itoa( root["ledOn"  ] | 2560, c, 10 );
    set( LEDON,    c );
    itoa( root["ledOff" ] | 2560, c, 10 );
    set( LEDOFF,   c );
  }
  f.close();
}
  
void Config::save() {
  if ( saved )
    return;
  SPIFFS.begin();
  if ( SPIFFS.exists( FPSTR( config_file ) ) ) {
    if ( !SPIFFS.rename( FPSTR( config_file ), F( "/growbox.old" ) ) )
      DEBUG_MSG( "Config::save error: Faild to rename file!\n" );  
  }
  File file = SPIFFS.open( FPSTR( config_file ), "w" );
  if ( !file ) {
    DEBUG_MSG( "Config::save error: Faild to open file!\n" );  
    return;
  }
  char temp[200];
  toJson( temp, 200 );
  file.print( temp );
  file.close();
  saved = true;
  DEBUG_MSG( "Config::save()\n" );  
}
