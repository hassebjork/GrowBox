#include <Arduino.h>
#include "GrowBox.h"
#include <Wire.h>

const char * fetName[] = { "Fan1", "Fan2", "Light", "Pump" };
const char   fetPin[]  = { 2, 0, 15, 13 };

GrowBox::GrowBox() {
  config.fetState    = 0;
  config.humidity    = 0;
  config.temperature = 0.0;

  // Initiate and switch all FETs off
  for ( char fetNo = 0; fetNo < sizeof( fetPin ) - 1; fetNo++ ) {
    pinMode( fetPin[fetNo], OUTPUT );
    digitalWrite( fetPin[fetNo], LOW );
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

uint8_t GrowBox::dht12get( uint8_t address ) {
  uint8_t data[5];
  
  Wire.beginTransmission( address );
  Wire.write(0);
  if ( Wire.endTransmission() != 0 )
    return 1; // Error connect
  
  Wire.requestFrom( address, (uint8_t) 5 );
  for ( uint8_t i = 0; i < 5; i++ )
    data[i] = Wire.read();
  delay( 50 );

  if ( Wire.available() != 0 )
    return 2; // Timeout

  if ( data[4]!=( data[0] + data[1] + data[2] + data[3] ) )
    return 3; // Checksum error
    
  config.humidity    = ( data[0] + (float) data[1] / 10 );
  config.temperature = ( data[2] + (float) data[3] / 10 );
  
  return 0;
}
