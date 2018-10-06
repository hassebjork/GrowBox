#include "GrowBox.h"

const char * fetName[] = { "Fan1", "Fan2", "Light", "Pump" };
const char   fetPin[]  = { 2, 0, 15, 13 };

GrowBox::GrowBox() {
  init();
}

void GrowBox::init() {
#ifdef COM
  Serial.begin( 115200 );
  Serial.println();
  Serial.println("Booting Sketch...");
#else
  pinMode( TX,  OUTPUT );
  pinMode( RX,  OUTPUT );
#endif

#ifdef I2C
  Wire.begin( SDA, SCL );
  oled.begin( &Adafruit128x64, I2C_OLED );
  oled.set400kHz();  
  oled.setFont(Adafruit5x7);  
#else
  pinMode( SDA, OUTPUT );
  pinMode( SCL, OUTPUT );
#endif
  
  pinMode( IO12, INPUT );
  pinMode( IO14, INPUT );
  
  config.fetState       = 0;
  config.humidity       = 50.0;
  config.temperature    = 20.0; // Predicted value
  config.previousMillis = 0;
  
  // Initiate and switch all FETs off
  for ( char fetNo = 0; fetNo < sizeof( fetPin ) - 1; fetNo++ ) {
    pinMode( fetPin[fetNo], OUTPUT );
    digitalWrite( fetPin[fetNo], LOW );
  }
  
}

void GrowBox::update() {
  unsigned long currentMillis = millis();
  char    curTime[13];
  char    buff[60];

  snprintf ( curTime, 60, "%02d %02d:%02d:%02d",
    (int)( currentMillis / 3600000 ) / 24, 
    (int)( currentMillis / 3600000 ) % 24,
    (int)( currentMillis / 60000 ) % 60,
    (int)( currentMillis / 1000 ) % 60 );

  
  if ( currentMillis - config.previousMillis >= INTERVAL ) {
    config.previousMillis += INTERVAL;
#ifdef I2C
    uint8_t t = dht12get();
    
    oled.home();
    oled.print("Time:");
    oled.setCursor( 40, 0 );
    oled.print( curTime );
    oled.clearToEOL();
    
    if ( t == 0 ) {
      oled.setCursor( 0, 1 );
      oled.print( "Temp: " );
      oled.setCursor( 40, 1 );
      oled.print( config.temperature, 1 );
      oled.clearToEOL();
      oled.setCursor( 0, 2 );
      oled.print( "Humid: " );
      oled.setCursor( 40, 2 );
      oled.print( config.humidity, 1 );
      oled.clearToEOL();
    } else {
      oled.setCursor( 0, 1 );
      oled.print( "DHT12 error: " );
      oled.println( t );
      oled.clearToEOL();
    }
#endif
  
#ifdef COM
    snprintf ( buff, 60, "\"%s\",%.1f,%.1f",
      curTime,
      config.temperature,
      config.humidity
    );
    Serial.println( buff );
#endif
    
  }
}

void GrowBox::fetSet( char fetNo, char value ) {
  if ( value )
    fetOn( fetNo );
  else
    fetOff( fetNo );
}

void GrowBox::fetOn( char fetNo ) {
  BIT_SET( config.fetState, fetNo );
  digitalWrite( fetPin[fetNo], HIGH );
}

void GrowBox::fetOff( char fetNo ) {
  BIT_CLEAR( config.fetState, fetNo );
  digitalWrite( fetPin[fetNo], LOW );
}

char GrowBox::fetStatus( char fetNo ) {
  return BIT_CHECK( config.fetState, fetNo );
}

uint8_t GrowBox::dht12get() {
  uint8_t data[5];
  
  Wire.beginTransmission( (uint8_t) I2C_DHT12 );
  Wire.write(0);
  if ( Wire.endTransmission() != 0 )
    return 1; // Error connect
  
  Wire.requestFrom( (uint8_t) I2C_DHT12, (uint8_t) 5 );
  for ( uint8_t i = 0; i < 5; i++ )
    data[i] = Wire.read();
  delay( 50 );

  if ( Wire.available() != 0 )
    return 2; // Timeout

  if ( data[4]!=( data[0] + data[1] + data[2] + data[3] ) )
    return 3; // Checksum error
    
  float temperature  = data[2] + (float) data[3] / 10;
  float humidity     = data[0] + (float) data[1] / 10;
  config.humidity    = 0.2 * humidity    + 0.8 * config.humidity;
  config.temperature = 0.2 * temperature + 0.8 * config.temperature;
  
  return 0;
}
