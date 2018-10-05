#include <Arduino.h>
#include "GrowBox.h"
#include <Wire.h>

const char * fetName[] = { "Fan1", "Fan2", "Light", "Pump" };
const char   fetPin[]  = { 2, 0, 15, 13 };

GrowBox::GrowBox() {
  fet      = 0;
  humidity = 0;
  temp     = 0.0;

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
  BIT_SET( fet, fetNo );
  digitalWrite( fetPin[fetNo], HIGH );
}

void GrowBox::fetOff( char fetNo ) {
  BIT_CLEAR( fet, fetNo );
  digitalWrite( fetPin[fetNo], LOW );
}

char GrowBox::fetStatus( char fetNo ) {
  return BIT_CHECK( fet, fetNo );
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
    
  humidity = ( data[0] + (float) data[1] / 10 );
  temp     = ( data[2] + (float) data[3] / 10 );
  
  return 0;
}
