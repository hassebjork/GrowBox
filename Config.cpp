#include "Config.h"

const char attr_name[]       PROGMEM = "name";
const char attr_tempMax[]    PROGMEM = "tempMax";
const char attr_humidMax[]   PROGMEM = "humidMax";
const char attr_tz[]         PROGMEM = "tz";
const char attr_dst[]        PROGMEM = "dst";
const char attr_ledOn[]      PROGMEM = "ledOn";
const char attr_ledOff[]     PROGMEM = "ledOff";
const char attr_logTime[]    PROGMEM = "logTime";
const char attr_updateTime[] PROGMEM = "updateTime";
const char attr_dimStep[]    PROGMEM = "dimStep";
const char* Config::attr[]   PROGMEM = {
  attr_name, attr_tempMax, attr_humidMax, attr_tz, attr_dst,
  attr_ledOn, attr_ledOff, attr_logTime, attr_updateTime,
  attr_dimStep
};

const char Config::config_file[] PROGMEM = "/growbox.json";

Config::Config() {
  ledOn.hour    = 26;
  ledOn.minute  = 60;
  ledOff.hour   = 26;
  ledOff.minute = 60;
  load();
}
Config::~Config() {
}
  
void Config::set( uint8_t d, const char *c ) {
  DEBUG_MSG("Config::set( %d, '%s' )\n", d, c );  
  
  switch( d ) {
    
    case NAME: {
      saved = ( strcmp( name, c ) == 0 );
      strlcpy( name, c, sizeof( name ) );
    }
    break;
    
    case TEMPMAX: {
      float f = atof( c );
      if ( f > 0.0 && f < 50.0 ) {
        saved = ( f == tempMax );
        tempMax = f;
      }
    }
    break;
    
    case HUMIDMAX: {
      uint8_t i = (uint8_t) atoi( c );
      if ( i >= 0 && i <= 100 ) {
        saved = ( i == humidMax );
        humidMax = i;
      }
    }
    break;
    
    case TZ: {
      int8_t i = atoi( c );
      if ( i > -13 && i < 13 ) {
        saved = ( i == tz );
        tz = i;
      }
    }
    break;
    
    case DST:
      saved = setBool( dst, c );
    break;
    
    case LEDON:
      saved = setAlarm( ledOn, c );
    break;
    
    case LEDOFF:
      saved = setAlarm( ledOff, c );
    break;
    
    case LOGTIME: {     // Set in seconds
      unsigned long i = atoi( c ) * 1000;
      if ( i >= 0 ) {
        saved = ( i == logTime );
        logTime = i;
      }
    }
    break;
    
    case UPDATETIME: {  // Set in seconds
      unsigned long i = atoi( c ) * 1000;
      if ( i > 0 ) {
        saved = ( i == updateTime );
        updateTime = i;
      }
    }
    break;
    
    case DIMSTEP: {
      uint8_t i = atoi( c );
      if ( i <= 200 ) {
        saved = ( i == dimStep );
        dimStep = i;
      }
    }
    break;
    
  }
}

bool Config::setBool( bool &b, const char *c ) {
  uint8_t i = atoi( c );
  bool saved;
  if ( i == 0 || i == 1 ) {
    saved = ( i == b );
    b = i;
  }
  return saved;
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

void Config::jsonAttribute( char *c, ATTR a, int size ) {
  strncat( c, "\"", size );
  strncat_P( c, attr[a], size );
  strncat( c, "\":", size );
}
void Config::jsonString( char *c, ATTR a, const char* s, int size ) {
  jsonAttribute( c, a, size );
  strncat( c, "\"", size );
  strncat( c, s, size );
  strncat( c, "\",", size );
}
void Config::jsonInt( char *c, ATTR a, int i, int size ) {
  char buff[10];
  jsonAttribute( c, a, size );
  itoa( i, buff, 10 );
  strncat( c, buff, size );
  strncat( c, ",", size );
}
void Config::jsonFloat( char *c, ATTR a, float f, int size ) {
  char buff[10];
  jsonAttribute( c, a, size );
  dtostrf( f, 0, 1, buff );
  strncat( c, buff, size );
  strncat( c, ",", size );
}

void Config::toJson( char *c, int size ) {
  char buff[10];
  strncpy( c, "{", size );
  jsonString( c, NAME, name, size );
  jsonInt( c, HUMIDMAX, humidMax, size );
  jsonFloat( c, TEMPMAX, tempMax, size );
  jsonInt( c, TZ, tz, size );
  jsonInt( c, DST, dst, size );
  jsonInt( c, LEDON, (uint16_t)(ledOn.hour * 100) + ledOn.minute, size );
  jsonInt( c, LEDOFF, (uint16_t)(ledOff.hour * 100) + ledOff.minute, size );
  jsonInt( c, LOGTIME, (int)( logTime / 1000 ), size );
  jsonInt( c, UPDATETIME, (int)( updateTime / 1000 ), size );
  jsonInt( c, DIMSTEP, dimStep, size );
  *(c + strlen( c ) - 1 ) = 0;
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
    set( NAME,       root[attr[NAME]]       | "DefName" );
    set( HUMIDMAX,   root[attr[HUMIDMAX]]   | "92" );
    set( TEMPMAX,    root[attr[TEMPMAX]]    | "28.0" );
    set( TZ,         root[attr[TZ]]         | "1"  );
    set( DST,        root[attr[DST]]        | "1"  );
    set( LOGTIME,    root[attr[LOGTIME]]    | "0"  );
    set( UPDATETIME, root[attr[UPDATETIME]] | "1"  );
    set( DIMSTEP,    root[attr[DIMSTEP]]    | "20" );
    itoa( root[attr[LEDON]  ] | 2560, c, 10 );
    set( LEDON,    c );
    itoa( root[attr[LEDOFF] ] | 2560, c, 10 );
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
