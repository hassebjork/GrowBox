/*
 * Light_Sleep WiFi
 * https://github.com/esp8266/Arduino/issues/1381#issuecomment-239519680
 * https://github.com/esp8266/Arduino/issues/1381#issuecomment-279117473
 * https://github.com/esp8266/Arduino/issues/1381#issuecomment-170139062
 * https://github.com/esp8266/Arduino/issues/1381#issuecomment-258621247
 * 
 * Light_Sleep GPIO_Wake
 * https://github.com/esp8266/Arduino/issues/1381#issuecomment-195303597
 * 
 */
#include "GrowBox.h"

const int     GrowBox::PWM_MAX     = 1023;
const char   *GrowBox::fetName[]   = { "led", "fan", "pump", "aux" };
const uint8_t GrowBox::fetPin[]    = { 15, 2, 0, 13 };
uint16_t      GrowBox::fetState[]  = { 0, 0, 0, 0 };  // Set value
uint16_t      GrowBox::fetValue[]  = { 0, 0, 0, 0 };  // Current value

GrowBox::GrowBox() {
  Wire.begin( SDA, SCL );
  oled.begin( &Adafruit128x32, I2C_OLED );
//  oled.begin( &Adafruit128x64, I2C_OLED );
  oled.set400kHz();  
  oled.setFont( Adafruit5x7 );
  oled.setScroll( false );
  oled.clear();
  
  uint8_t i = dht12get( temperature, humidity );

  millisUpd  = 0;
  analogWriteFreq( 50000 );
  analogWriteRange( PWM_MAX );
  
  // Initiate and switch all FETs off
  for ( i = 0; i < sizeof( fetPin); i++ ) {
    pinMode( fetPin[i], OUTPUT );
    setValue( i, 0 );
    fetState[i] = 0;
  }
}

void GrowBox::update() {
  float t, h;
  int fan, led;
  
  if ( dht12get( t, h ) == 0 ) {
    temperature = t * 0.4 + temperature * 0.6;
    humidity    = h * 0.4 + humidity    * 0.6;

    // Update log data
    logTemp    += t;
    logHumid   += h;
    if ( t > maxTemp  ) maxTemp  = t;
    if ( t < minTemp  ) minTemp  = t;
    if ( h > maxHumid ) maxHumid = t;
    if ( h < minHumid ) minHumid = t;
    logCount++;
  }
  
  // Temp max + 2C
  if ( temperature > config.tempMax + 2.0 ) {
    setValue( FAN, PWM_MAX );
    dim( LED, -1 );

  // Temp Max
  } else if ( temperature > config.tempMax ) {
    dim( LED );
    dim( FAN, 1 );

  // Normal temp
  } else {
    if ( humidity > config.humidMax ) {
      dim( FAN, 1 );
  } else {
      dim( LED );
      dim( FAN );
    }
  }
}

void GrowBox::logRecord() {
  if ( logCount > 0 ) {
    logTemp  = logTemp  / logCount;
    logHumid = logHumid / logCount;

    // Log function here
  }

  // Reset log values
  logTemp  = 0.0;
  maxTemp  = -300.0;
  minTemp  = 400;
  logHumid = 0.0;
  maxHumid = 0.0;
  minHumid = 100.0;
  logCount = 0;
}

void GrowBox::setValue( uint8_t no, uint16_t value ) {
  if ( value > PWM_MAX ) {
    fetValue[no] = PWM_MAX;
    analogWrite( fetPin[no], PWM_MAX );
  } else {
    fetValue[no] = value;
    analogWrite( fetPin[no], value );
  }
}

void GrowBox::dim( uint8_t no, int8_t  v ) {
  int l = fetValue[no];

  // Stabilize
  if ( v == 0 ) {
    if ( l == fetState[no] )
      return;
    
    if ( l < fetState[no] ) {
      l += DIM_STEP;
      setValue( no, l > fetState[no] ? fetState[no] : l );
    } else if ( l > fetState[no] ) {
      if ( l < DIM_STEP ) {
        setValue( no, 0 );
      } else {
        l -= DIM_STEP;
        setValue( no, l < fetState[no] ? fetState[no] : l );
      }
    }
  
  // Increase
  } else if ( v > 0 ) {
    l += DIM_STEP;
    setValue( no, l > PWM_MAX ? PWM_MAX : l );

  // Decrease
  } else if ( v < 0 ) {
    if ( l < DIM_STEP ) {
      setValue( no, 0 );
    } else {
      l -= DIM_STEP;
      setValue( no, l < 0 ? 0 : l );
    }
  }
}

void GrowBox::toJson( char *c, int size ) {
  char buff[10];
  strncpy( c, "{\"temp\":", size ); 
  dtostrf( temperature, 0, 1, buff );
  strncat( c, buff, size ); 
  strncat( c, ",\"humid\":", size ); 
  dtostrf( humidity, 0, 1, buff );
  strncat( c, buff, size ); 
    
  for ( uint8_t i = 0; i < sizeof( fetPin); i++ ) {
    strncat( c, ",\"", size );
    strncat( c, fetName[i], size );
    strncat( c, "\":", size );
    itoa( fetStatus(i), buff, 10 );
    strncat( c, buff, size );
    
    strncat( c, ",\"a", size );
    strncat( c, fetName[i], size );
    strncat( c, "\":", size );
    itoa( fetValue[i], buff, 10 );
    strncat( c, buff, size );
  }
  
  strncat( c, "}", size ); 
  
//  DEBUG_MSG("GrowBox::toJson: %d bytes\n", strlen( c ) );
}

void GrowBox::fetSet( uint8_t no, uint16_t value ) {
  if ( no < sizeof( fetPin ) ) {
    if ( value > PWM_MAX )
      value = PWM_MAX;
    fetState[no] = value;
    setValue( no, value );
  }
}

uint16_t GrowBox::fetStatus( uint8_t no ) {
  return fetState[no];
}

uint8_t GrowBox::dht12get( float &t, float &h ) {
  uint8_t buf[5];
  t = 0.0;
  h = 0.0;
  
  Wire.beginTransmission( (uint8_t) I2C_DHT12 );
  Wire.write(0);
  if ( Wire.endTransmission() != 0 )
    return 1; // Error connect
  
  Wire.requestFrom( (uint8_t) I2C_DHT12, (uint8_t) 5 );
  for ( uint8_t i = 0; i < 5; i++ )
    buf[i] = Wire.read();
  delay( 50 );

  if ( Wire.available() != 0 )
    return 2; // Timeout

  if ( buf[4]!=( buf[0] + buf[1] + buf[2] + buf[3] ) )
    return 3; // Checksum error

  h = ( buf[0] + (float) buf[1] / 10 );
  t = ( buf[2] + (float) buf[3] / 10 );
  return 0;
}
