#include "Config.h"
const char month_jan[] PROGMEM = "Jan";
const char month_feb[] PROGMEM = "Feb";
const char month_mar[] PROGMEM = "Mar";
const char month_apr[] PROGMEM = "Apr";
const char month_may[] PROGMEM = "May";
const char month_jun[] PROGMEM = "Jun";
const char month_jul[] PROGMEM = "Jul";
const char month_aug[] PROGMEM = "Aug";
const char month_sep[] PROGMEM = "Sep";
const char month_oct[] PROGMEM = "Oct";
const char month_nov[] PROGMEM = "Nov";
const char month_dec[] PROGMEM = "Dec";
const char* Config::months[] PROGMEM = {
	month_jan, month_feb, month_mar, month_apr, month_may, month_jun,
	month_jul, month_aug, month_sep, month_oct, month_nov, month_dec
};

const char attr_name[]       PROGMEM = "name";
const char attr_tempMax[]    PROGMEM = "tempMax";
const char attr_humidMax[]   PROGMEM = "humidMax";
const char attr_tz[]         PROGMEM = "tz";
const char attr_dst[]        PROGMEM = "dst";
const char attr_ledOn[]      PROGMEM = "ledOn";
const char attr_ledOff[]     PROGMEM = "ledOff";
const char attr_logTime[]    PROGMEM = "logTime";
const char* Config::attr[]   PROGMEM = {
	attr_name, attr_tempMax, attr_humidMax, attr_tz,
	attr_dst, attr_ledOn, attr_ledOff, attr_logTime
};

const char Config::config_file[] PROGMEM = "/growbox.json";
const char Config::config_old[]  PROGMEM = "/growbox.old";

Config::Config() {
	ledOn.hour    = 6;
	ledOn.minute  = 0;
	ledOff.hour   = 20;
	ledOff.minute = 0;
	load();
}
Config::~Config() {
}

void Config::set( const char *key, const char *value ) {
	DEBUG_MSG("Config::set( '%s', '%s' )\n", key, value );  
	for ( uint8_t i = NAME; i < attrNo; i++ ) {
		if ( strcmp( key, attr[i] ) == 0 ) {
			set( i, value );
			break;
		}
	}
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
			unsigned long i = atol( c ) * 1000;
			saved = ( i == logTime );
			logTime = i;
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

time_t Config::today( uint16_t hrmin = 0 ) {
	return today( (uint8_t) hrmin / 100, (uint8_t) hrmin % 100, 0 );
}

time_t Config::today( uint8_t hr = 0, uint8_t min = 0, uint8_t sec = 0 ) {
	tmElements_t tm;
	tm.Year   = year( time );
	tm.Month  = month( time );
	tm.Day    = day( time );
	tm.Hour   = hr;
	tm.Minute = min;
	tm.Second = sec;
	return makeTime( tm );
}

bool Config::checkDst() {
	if ( !dst ) 
		return false;
	time_t t = now() + tz * SECS_PER_HOUR;
	if ( month(t)  < 3 || month(t) > 10 )       // Jan, Feb, Nov, Dec 
		return false;
	if ( month(t)  > 3 && month(t) < 10 )       // Apr, Jun; Jul, Aug, Sep 
		return true;
	if ( month(t) ==  3 && ( hour(t) + 24 * day(t) ) >= ( 3 +  24 * ( 31 - ( 5 * year(t) / 4 + 4 ) % 7 ) ) 
		|| month(t) == 10 && ( hour(t) + 24 * day(t) ) <  ( 3 +  24 * ( 31 - ( 5 * year(t) / 4 + 1 ) % 7 ) ) )
		return true;
	else
		return false;
}

void Config::timeRefresh() {
	time = now() 
		+ ( tz * SECS_PER_HOUR )
		+ ( checkDst() ? SECS_PER_HOUR : 0 );
}

void Config::toJson( char *c, int size ) {
	strncat( c, "{", size );
	toJson( c, NAME, name, size );
	toJson( c, HUMIDMAX, humidMax, size );
	toJson( c, TEMPMAX, tempMax, size );
	toJson( c, TZ, tz, size );
	toJson( c, DST, dst, size );
	toJson( c, LEDON, (uint16_t)(ledOn.hour * 100) + ledOn.minute, size );
	toJson( c, LEDOFF, (uint16_t)(ledOff.hour * 100) + ledOff.minute, size );
	toJson( c, LOGTIME, (int)( logTime / 1000 ), size );
	strncat( c, "}", size );
}
void Config::jsonAttribute( char *c, ATTR a, int size ) {
	jsonAttribute( c, attr[a], a != NAME, size );
}
void Config::jsonAttribute( char *c, const char *a, bool k, int size ) {
	if ( k )
		strncat( c, ",", size );
	toJson( c, a, size );
	strncat( c, ":", size );
}
// String
void Config::toJson( char *c, const char* s, int size ) {
	strncat( c, "\"", size );
	strncat( c, s, size );
	strncat( c, "\"", size );
}
void Config::toJson( char *c, ATTR a, const char* s, int size ) {
	jsonAttribute( c, a, size );
	toJson( c, s, size );
}
// Integer
void Config::toJson( char *c, int i, int size ) {
	char buff[10];
	itoa( i, buff, 10 );
	strncat( c, buff, size );
}
void Config::toJson( char *c, ATTR a, int i, int size ) {
	jsonAttribute( c, a, size );
	toJson( c, i, size );
}
// Float
void Config::toJson( char *c, float f, int size ) {
	char buff[10];
	dtostrf( f, 0, 1, buff );
	strncat( c, buff, size );
}
void Config::toJson( char *c, ATTR a, float f, int size ) {
	jsonAttribute( c, a, size );
	toJson( c, f, size );
}

void Config::load() {
	DEBUG_MSG("Config::load()\n" );  
	SPIFFS.begin();
	File f = SPIFFS.open( FPSTR( config_file ), "r" );
	if ( !f ) {
		DEBUG_MSG("Config::load error: No file!\n" );  
		return;
	}
	
	JsonStreamingParser parser;
	parser.setListener( this );
	
	while( f.available() )
		parser.parse( f.read() );
	
	f.close();
}
  
void Config::save() {
	if ( saved )
		return;
	SPIFFS.begin();
	if ( SPIFFS.exists( FPSTR( config_file ) ) ) {
		if ( SPIFFS.exists( FPSTR( config_old ) ) )
			SPIFFS.remove( FPSTR( config_old ) );
		if ( !SPIFFS.rename( FPSTR( config_file ), FPSTR( config_old ) ) )
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

void Config::key( String key ) {
	_key = key;
}
void Config::value( String value ) {
	set( _key.c_str(), value.c_str() );
}
void Config::whitespace( char c ) {}
void Config::startDocument() {}
void Config::endArray() {}
void Config::endObject() {}
void Config::endDocument() {}
void Config::startArray() {}
void Config::startObject() {}
