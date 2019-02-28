#ifndef _Log_h
#define _Log_h

#define MAX_TEMP -300.0
#define MIN_TEMP  400.0
#define MAX_HUMID   0.0
#define MIN_HUMID 101.0

#include <TimeLib.h>            // https://github.com/PaulStoffregen/Time
#include "Config.h"             // Configuration class for local storage
#include "FS.h"                 // https://github.com/esp8266/Arduino/tree/master/cores/esp8266
extern Config config;

class Log {
public:
	enum STATE {
		STATE_OK, STATE_UPDATE
	};
	float    temp;
	float    tempMax;
	float    tempMin;
	float    humid;
	float    humidMax;
	float    humidMin;
	unsigned int count;
	
	unsigned long previous;		// Time of last update
	
	Log();
	~Log();
	
	uint8_t update( float t, float h );
	void clear();
	void logFile();
};
#endif
