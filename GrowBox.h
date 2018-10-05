#ifndef _GrowBox_h
#define _GrowBox_h

#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

#define FAN1 0
#define FAN2 1
#define LED  2
#define AUX  3

typedef struct {
  uint8_t fetState;     // Last state of FETs
  float   humidity;     // Air humidity
  float   temperature;  // Air temperature
  uint8_t minHumid;
  uint8_t maxHumid;
  float   minTemp;
  float   maxTemp;
  long    interval;    // Update freq
} Config;

class GrowBox {
public:
  Config config;
  float   temp;     // Air Temperature 
  
  GrowBox();
  void fetSet( char fetNo, char stat );
  void fetOn( char fetNo );
  void fetOff( char fetNo );
  char fetStatus( char fetNo );
  
  uint8_t dht12get( uint8_t address );
};

#endif
