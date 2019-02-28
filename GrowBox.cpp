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

const char    GrowBox::TEMP[]      = "temp";
const char    GrowBox::HUMID[]     = "humid";
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
  
#ifdef AM2320
  uint8_t i = am2320get( temperature, humidity );
#else
  uint8_t i = dht12get( temperature, humidity );
#endif
  
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
  
#ifdef AM2320
  if ( am2320get( t, h ) == NO_ERROR ) {
#else
  if ( dht12get( t, h ) == NO_ERROR ) {
#endif
    temperature = t * 0.4 + temperature * 0.6;
    humidity    = h * 0.4 + humidity    * 0.6;

    // Update log data
	log.update( t, h );
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

void GrowBox::setValue( uint8_t no, uint16_t value ) {
  fetValue[no] = ( value > PWM_MAX ? PWM_MAX : value );
  analogWrite( fetPin[no], fetValue[no] );
//   DEBUG_MSG("GrowBox::setValue: %d = %d\n", fetPin[no], fetValue[no] );
}

void GrowBox::dim( uint8_t no, int8_t  v ) {
  int l = fetValue[no];

  // Stabilize
  if ( v == 0 ) {
    if ( l == fetState[no] )
      return;
    
    if ( l < fetState[no] ) {
      l += config.dimStep;
      setValue( no, l > fetState[no] ? fetState[no] : l );
    } else if ( l > fetState[no] ) {
      if ( l < config.dimStep ) {
        setValue( no, 0 );
      } else {
        l -= config.dimStep;
        setValue( no, l < fetState[no] ? fetState[no] : l );
      }
    }
  
  // Increase
  } else if ( v > 0 ) {
    l += config.dimStep;
    setValue( no, l > PWM_MAX ? PWM_MAX : l );

  // Decrease
  } else if ( v < 0 ) {
    if ( l < config.dimStep ) {
      setValue( no, 0 );
    } else {
      l -= config.dimStep;
      setValue( no, l < 0 ? 0 : l );
    }
  }
}

void GrowBox::toJson( char *c, int size ) {
  strncat( c, "{", size );
  Config::jsonAttribute( c, TEMP, false, size );
  Config::toJson( c, temperature, size );
  Config::jsonAttribute( c, HUMID, true, size );
  Config::toJson( c, humidity, size );
    
  for ( uint8_t i = 0; i < sizeof(fetPin); i++ ) {
	Config::jsonAttribute( c, fetName[i], true, size );
    strncat( c, "[", size );
	Config::toJson( c, fetState[i], size );
    strncat( c, ",", size );
	Config::toJson( c, fetValue[i], size );
    strncat( c, "]", size );
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
    return ERROR_CONNECT;
  
  Wire.requestFrom( (uint8_t) I2C_DHT12, (uint8_t) 5 );
  for ( uint8_t i = 0; i < 5; i++ )
    buf[i] = Wire.read();
  delay( 50 );

  if ( Wire.available() != 0 )
    return ERROR_TIMEOUT;

  if ( buf[4]!=( buf[0] + buf[1] + buf[2] + buf[3] ) )
    return ERROR_CHECKSUM;

  h = ( buf[0] + (float) buf[1] / 10 );
  t = ( buf[2] + (float) buf[3] / 10 );
  return NO_ERROR;
}

#ifdef AM2320
uint8_t GrowBox::am2320get( float &t, float &h ) {
	uint8_t buf[8];
	uint16_t crc;
	// Wake sensor
	Wire.beginTransmission( I2C_AM2320 );
	Wire.write( 0x00 );
	Wire.endTransmission();
	delay( 10 );
	
	// Send read command
	Wire.beginTransmission( I2C_AM2320 );	
	Wire.write( 0x03 );		// Function code
	Wire.write( 0x00 );		// Start address
	Wire.write( 0x04 );		// No. bytes
	if ( Wire.endTransmission() != 0 )
		return ERROR_CONNECT;
	delay( 2 );
	
	// Request 
	Wire.requestFrom( I2C_AM2320, 0x08);
	for ( uint8_t i = 0; i < 8; i++ )
		buf[i] = Wire.read();
	
	crc = ( buf[7] << 8 ) + buf[6];
	if ( crc == crc16( buf, 6 ) ) {
		t = ( ( ( buf[4] & 0x7F ) << 8 ) + buf[5] ) / 10.0;
		if ( buf[4] & 0x80 )
			t = -t;
		h = ( ( buf[2] << 8 ) + buf[3] ) / 10.0;
		return NO_ERROR;
	}
    return ERROR_CHECKSUM;
}

uint16_t GrowBox::crc16( uint8_t *buf, uint8_t no ) {
	uint8_t  i;
	uint16_t crc = 0xFFFF;
	
	while ( no-- ) {
		crc ^= *buf++;
		for ( i = 0; i < 8; i++ ) {
			if ( crc & 0x0001 ) {
				crc >>= 1;
				crc ^= 0xA001;
			} else {
				crc >>= 1;
			}
		}
	}
	return crc;
}
#endif
