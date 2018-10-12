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
  oled.setFont( Adafruit5x7 );
  oled.clear();
#ifdef COM
  Serial.println("Init OLED");
#endif
#else
  pinMode( SDA, OUTPUT );
  pinMode( SCL, OUTPUT );
#ifdef COM
  Serial.println("Skipping WIRE");
#endif
#endif
  
  config.read( "/config.json" );
  delay( 100 );
  
  pinMode( IO12, INPUT );
  pinMode( IO14, INPUT );

  logCount = 0;
  logTemp  = 0.0;
  logHumid = 0.0;
  
  data.fetState       = 0;
  data.humidity       = 50.0;
  data.temperature    = 20.0;
  data.previousMillis = 0;

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
  uint8_t sec = ( currentMillis / 1000    ) % 60;
  uint8_t min = ( currentMillis / 60000   ) % 60;
  uint8_t hr  = ( currentMillis / 3600000 ) % 24;
  uint8_t day = ( currentMillis / 3600000 ) / 24;
  
  snprintf ( curTime, 60, "%02d %02d:%02d:%02d",
    day, hr, min, sec );
  
  if ( currentMillis - data.previousMillis >= INTERVAL ) {
    data.previousMillis += INTERVAL;
  
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
      oled.print( data.temperature, 1 );
      oled.clearToEOL();
      oled.setCursor( 0, 2 );
      oled.print( "Humid: " );
      oled.setCursor( 40, 2 );
      oled.print( data.humidity, 1 );
      oled.clearToEOL();

      logTemp  += data.temperature;
      logHumid += data.humidity;
      logCount++;
    } else {
      oled.setCursor( 0, 1 );
      oled.print( String( "DHT12 error: " ) + t );
#ifdef COM
      Serial.println( String( "DHT12 error: " ) + t );
#endif
      oled.clearToEOL();
    }

    if ( sec == 0 && min == 0 ) {
      File f = SPIFFS.open( "/log.txt", "a" );
      if ( !f ) {
        oled.setCursor( 0, 6 );
        oled.print( "Failed to open log.txt" );
#ifdef COM
        Serial.println( "Failed to open log.txt" );
#endif
      } else {
        if ( logCount > 0 ) {
          snprintf ( buff, 60, "\"%s\",%.1f,%.1f",
            curTime, logTemp / logCount, data.humidity );
//          f.println( buff );
          oled.setCursor( 0, 4 );
          oled.print( buff );
          oled.clearToEOL();
        }
        f.close();
        logCount = 0;
        logTemp  = 0.0;
        logHumid = 0.0;
      }
    }
#endif
  
#ifdef COM
    snprintf ( buff, 60, "\"%s\",%.1f,%.1f",
      curTime,
      data.temperature,
      data.humidity
    );
    Serial.println( buff );
    snprintf ( buff, 60, "Name:\"%s\" id:%d minT:%.1f maxT:%.1f minH:%d maxH:%d",
      config.name,
      config.id,
      config.minTemp,
      config.maxTemp,
      config.minHumid,
      config.maxHumid
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
  BIT_SET( data.fetState, fetNo );
  digitalWrite( fetPin[fetNo], HIGH );
}

void GrowBox::fetOff( char fetNo ) {
  BIT_CLEAR( data.fetState, fetNo );
  digitalWrite( fetPin[fetNo], LOW );
}

char GrowBox::fetStatus( char fetNo ) {
  return BIT_CHECK( data.fetState, fetNo );
}

uint8_t GrowBox::dht12get() {
  uint8_t buf[5];
  
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
    
  data.humidity     = 0.2 * (buf[0] + (float) buf[1] / 10) + 0.8 * data.humidity;
  data.temperature  = 0.2 * (buf[2] + (float) buf[3] / 10) + 0.8 * data.temperature;
  
  return 0;
}
