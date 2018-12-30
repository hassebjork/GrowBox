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

const int     GrowBox::PWM_MAX     = 1022;
const char   *GrowBox::fetName[]   = { "led", "fan1", "fan2", "aux" };
const uint8_t GrowBox::fetPin[]    = { 15, 2, 0, 13 };
uint16_t      GrowBox::fetState[]  = { 0, 0, 0, 0 };

GrowBox::GrowBox() {
  init();
}

void GrowBox::init() {
  Wire.begin( SDA, SCL );
  oled.begin( &Adafruit128x32, I2C_OLED );
//  oled.begin( &Adafruit128x64, I2C_OLED );
  oled.set400kHz();  
  oled.setFont( Adafruit5x7 );
  oled.setScroll( false );
  oled.clear();
  
  uint8_t i = dht12get( temperature, humidity );

  millisUpd  = 0;
  logMillis  = millis();
  logCount   = 1;
  logTemp    = temperature;
  logHumid   = humidity;
  analogWriteFreq( 25000 );
  
  // Initiate and switch all FETs off
  for ( i = 0; i < sizeof( fetPin); i++ ) {
    pinMode( fetPin[i], OUTPUT );
    analogWrite( fetPin[i], 0 );
    fetState[i] = 0;
  }
}

void GrowBox::doActivate() {
  if ( temperature > config.tempMax )
    ;
}

void GrowBox::update() {
  unsigned long millisCur = millis();
  
  if ( millisCur - millisUpd >= INTERVAL_UPD ) {
    millisUpd += INTERVAL_UPD;
    
    float t, h;
    uint8_t i = dht12get( t, h );
    int fan, led;
    
    if ( i == 0 ) {
      temperature = t * 0.2 + temperature * 0.8;
      humidity    = h * 0.2 + humidity    * 0.8;
      logTemp  += temperature;
      logHumid += humidity;
      logCount++;
    }

    
    // Temp max + 2C
    if ( temperature > config.tempMax + 2.0 ) {
      analogWrite( fetPin[GrowBox::FAN1], GrowBox::PWM_MAX );
      dim( GrowBox::LED, -10 );

    // Temp Max
    } else if ( temperature > config.tempMax ) {
      dim( GrowBox::LED );
      dim( GrowBox::FAN1, +10 );

    // Normal temp
    } else {
      if ( humidity > config.humidMax )
        dim( GrowBox::FAN1, +10 );
      else {
        dim( GrowBox::LED );
        dim( GrowBox::FAN1 );
      }
    }
   }
    
  if ( millisCur - millisUpd >= INTERVAL_CALC ) {
      logMillis += INTERVAL_CALC;
      logCount = 0;
      logTemp  = 0.0;
      logHumid = 0.0;
  }
//    if ( second( time ) == 0 && minute( time ) == 0 ) {
//      File f = SPIFFS.open( F("/log.txt"), "a" );
//      if ( !f ) {
//        oled.setCursor( 0, 6 );
//        oled.print( F("Failed to open log.txt") );
//#ifdef COM
//        _s.println( F("Failed to open log.txt") );
//#endif
//      } else {
//        if ( logCount > 0 ) {
//          snprintf ( buff, 60, "\"%s\",%.1f,%.1f",
//            curTime, logTemp / logCount, humidity );
////          f.println( buff );
//          oled.setCursor( 0, 4 );
//          oled.print( buff );
//          oled.clearToEOL();
//        }
//        f.close();
//      }
//      logCount = 0;
//      logTemp  = 0.0;
//      logHumid = 0.0;
//    }
}

void GrowBox::dim( uint8_t no, int8_t  v ) {
    int l = analogRead( fetPin[no] );
    if ( v == 0 ) {
      if ( l > fetState[no] )
        analogWrite( fetPin[no], ( l - 10 > fetState[no] ? l - 10 : fetState[no] ) );
      else if ( l < fetState[no] )
        analogWrite( fetPin[no], ( l + 10 < fetState[no] ? l + 10 : fetState[no] ) );
    } else {
      if ( l + v < 0 ) 
        analogWrite( fetPin[no], 0 );
      else if ( l + v > GrowBox::PWM_MAX )
        analogWrite( fetPin[no], GrowBox::PWM_MAX );
      else
        analogWrite( fetPin[no], l + v );
    }
}

void GrowBox::toJson( char *c, int size ) {
  char buff[20];
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
    strncat( c, buff, size ); // 0-1023
  }
  
  strncat( c, ",\"uptime\":", size ); 
  itoa( millis(), buff, 10 );
  strncat( c, buff, size ); 
  
  strncat( c, "}", size ); 
}

void GrowBox::fetSet( uint8_t no, uint16_t value ) {
  if ( no < sizeof( fetPin ) && no > 0 ) {
    if ( value < 0 )       value = 0;
    if ( value > GrowBox::PWM_MAX ) value = GrowBox::PWM_MAX;
    fetState[no] = value;
    analogWrite( fetPin[no], fetState[no] );
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


