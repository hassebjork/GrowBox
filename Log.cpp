#include "Log.h"

const char LOG_DIR[]  = "/log";

Log::Log() {
	clear();
	previous = millis();
}
Log::~Log() {
}

uint8_t Log::update( float t, float h ) {
	if ( config.logTime == 0 )
		return STATE_OK;
	temp += t;
	if ( t > tempMax ) tempMax = t;
	if ( t < tempMin ) tempMin = t;
	
	humid += h;
	if ( h > humidMax ) humidMax = h;
	if ( h < humidMin ) humidMin = h;
	
	count++;
	if ( config.logTime > 0 && millis() - previous >= config.logTime ) {
		previous += config.logTime;
		logFile();
		clear();
		return STATE_UPDATE;
	}
	return STATE_OK;
}

void Log::clear() {
	temp     = 0.0;
	tempMax  = MAX_TEMP;
	tempMin  = MIN_TEMP;
	humid    = 0.0;
	humidMax = MAX_HUMID;
	humidMin = MIN_HUMID;
	count    = 0;
}

void Log::logFile() {	// Time in UTC
	if ( count > 0 ) {
		char filename[18];
		SPIFFS.begin();
		snprintf_P( filename, sizeof(filename), "%s%d%s.json", 
				LOG_DIR, year() - 2000, Config::months[month()-1] );
		bool exist = SPIFFS.exists( filename );
		File file = SPIFFS.open( filename, "a" );
		
		file.printf( "%s[%d%02d%02d,%d,[%.1f,%.1f,%.1f],[%.1f,%.1f,%.1f]]", 
			( exist ? "," : "" ),
			year( ) - 2000, month( ), day(),
			( hour() * 100 + minute() ),
			tempMin,  temp / count,  tempMax, 
			humidMin, humid / count, humidMax
		);
		file.close();
	}
}

