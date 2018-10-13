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
#include <ESP8266WiFi.h>
#include "GrowBox.h"

const char * fetName[] = { "Fan1", "Fan2", "Light", "Pump" };
const char   fetPin[]  = { 2, 0, 15, 13 };

GrowBox::GrowBox() {
  init();
}

void GrowBox::init() {
#ifdef COM
  Serial.begin( 115200 );
  _s.println();
  _s.println("Booting Sketch...");
#else
  pinMode( TX,  OUTPUT );
  pinMode( RX,  OUTPUT );
#endif

#ifdef I2C
  Wire.begin( SDA, SCL );
  oled.begin( &Adafruit128x64, I2C_OLED );
  oled.set400kHz();  
  oled.setFont( Adafruit5x7 );
  oled.setScroll( true );
  oled.clear();
  oled.println( "GrowBox" );
#ifdef COM
  _s.println("Init OLED");
#endif
#else
  pinMode( SDA, OUTPUT );
  pinMode( SCL, OUTPUT );
#ifdef COM
  _s.println("Skipping WIRE");
#endif
#endif
  
  config.read( "/config.json" );
  delay( 100 );
  
  WiFi.mode( WIFI_STA );
  WiFi.begin( config.ssid, config.pass );

  uint8_t i = 0;
  oled.print( "WiFi" ); 
  while ( WiFi.status() != WL_CONNECTED && i++ < 100 ) {
     delay( 500 );
     oled.print( "." );
  }
  oled.println( "\nConnected!" ); 
  oled.println( WiFi.localIP() );
  
  /* Time */  
  setSyncInterval( 24*60*60 );  // Daily
  setSyncProvider( getNtpTime );
  
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
  delay( 1000 );
  oled.clear();
}

void GrowBox::update() {
  unsigned long currentMillis = millis();
  char    curTime[20];
  char    buff[60];
  
  time_t time = now() 
    + config.tz * SECS_PER_HOUR
    + ( dst( now(), config.tz ) ? 1 : 0 ) * SECS_PER_HOUR;
  
  snprintf ( curTime, sizeof(curTime), "%02d-%02d-%02d %02d:%02d:%02d",
    year( time ), month( time ), day( time ),
    hour( time ), minute( time ), second( time ) );
  
  oled.home();
  oled.print( curTime );
  oled.clearToEOL();
  
  if ( currentMillis - data.previousMillis >= INTERVAL ) {
    data.previousMillis += INTERVAL;
  
#ifdef I2C
    uint8_t t = dht12get();
    
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
      _s.println( String( "DHT12 error: " ) + t );
#endif
      oled.clearToEOL();
    }

    if ( second( time ) == 0 && minute( time ) == 0 ) {
      File f = SPIFFS.open( "/log.txt", "a" );
      if ( !f ) {
        oled.setCursor( 0, 6 );
        oled.print( "Failed to open log.txt" );
#ifdef COM
        _s.println( "Failed to open log.txt" );
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
      }
      logCount = 0;
      logTemp  = 0.0;
      logHumid = 0.0;
    }
#endif

  }
  delay( 900 );
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

time_t getNtpTime() {
  IPAddress   timeServer;
  const char* ntpServerName = NTPSERVER;
  byte        packetBuffer[NTP_PACKET_SIZE];
  WiFiUDP     Udp;
  
  Udp.begin( TIMEPORT );
  while ( Udp.parsePacket() > 0 );    // discard any previously received packets
  WiFi.hostByName( ntpServerName, timeServer ); 
  
  // send an NTP request to the time server at the given address
  // set all bytes in the buffer to 0
  memset( packetBuffer, 0, NTP_PACKET_SIZE );
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket( timeServer, 123 ); //NTP requests are to port 123
  Udp.write( packetBuffer, NTP_PACKET_SIZE );
  Udp.endPacket();
  
  uint32_t beginWait = millis();
  while ( millis() - beginWait < 1500 ) {
    int size = Udp.parsePacket();
    if ( size >= NTP_PACKET_SIZE ) {
      Udp.read( packetBuffer, NTP_PACKET_SIZE );  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL;
    }
  }
  return 0; // return 0 if unable to get the time
}

bool dst( time_t t, uint8_t tz ) {
  t += tz * SECS_PER_HOUR;
  if ( month(t)  < 3 || month(t) > 10 )       // Jan, Feb, Nov, Dec 
    return false;
  if ( month(t)  > 3 && month(t) < 10 )       // Apr, Jun; Jul, Aug, Sep 
    return true;
  if ( month(t) ==  3 && ( hour(t) + 24 * day(t) ) >= ( 3 +  24 * ( 31 - ( 5 * year(t) / 4 + 4 ) % 7 ) ) 
    || month(t) == 10 && ( hour(t) + 24 * day(t) ) <  ( 3 +  24 * ( 31 - ( 5 * year(t) / 4 + 1 ) % 7 ) ) )
    return true;
  else
    return false;
}

