/* Strings
 *   https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
 * 
 */

#include "Config.h"                         // Configuration class for local storage
Config config;

#include "GrowBox.h"
GrowBox growBox;

#include <TimeLib.h>      // https://github.com/PaulStoffregen/Time
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char * headerKeys[] = { "date" };
const size_t numberOfHeaders = 1;
const char * _months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }; 

#include <pgmspace.h>                       // PSTR & PROGMEM

#include <math.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
extern "C" {
  #include "user_interface.h"
}
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiEventHandler disconnectedEventHandler;  // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-examples.html#event-driven-methods

const char spinner[] = ".oOo";

time_t syncHTTP() {
  tmElements_t tm;
  if ( WiFi.status() == WL_CONNECTED ) {
    HTTPClient http;
    http.begin( "http://google.com/" ); 
    http.collectHeaders( headerKeys, numberOfHeaders );
    
    if ( http.GET() > 0 ) {
      uint8_t i;
      String headerDate = http.header("date");
      const char * str  = headerDate.c_str();
      const char * mo   = headerDate.substring( 8, 11 ).c_str();
      
      for ( i = 0; i < 12; i++ ) {
        if ( strncmp( _months[i], mo, 3 ) == 0 )
          break;
      }
      
      tm.Year   = ( str[14] - '0' ) * 10 + ( str[15] - '0' ) + 30;
      tm.Month  = i;
      tm.Day    = ( str[5]  - '0' ) * 10 + ( str[6]  - '0' );
      tm.Hour   = ( str[17] - '0' ) * 10 + ( str[18] - '0' );
      tm.Minute = ( str[20] - '0' ) * 10 + ( str[21] - '0' );
      tm.Second = ( str[23] - '0' ) * 10 + ( str[24] - '0' );
    }
    http.end();
  } else {
    tm.Year   = 2000;
    tm.Month  = 1;
    tm.Day    = 1;
    tm.Hour   = 0;
    tm.Minute = 0;
    tm.Second = 0;
  }
  return makeTime( tm );
}
bool checkDst() {
  if ( !config.dst ) 
    return false;
  time_t t = now() + config.tz * SECS_PER_HOUR;
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

/* TODO: Add limits */
void argFloat( float &f, const char *s ) {
  if ( server.hasArg( s ) )
    f = atof( server.arg( s ).c_str() );
}
void argInt( uint8_t &f, const char *s ) {
  if ( server.hasArg( s ) )
    f = atoi( server.arg( s ).c_str() );
}
void argInt( int8_t &f, const char *s ) {
  if ( server.hasArg( s ) )
    f = atoi( server.arg( s ).c_str() );
}

void handleIO() {
  int t;
  for ( uint8_t i = 0; i < GrowBox::fetNo; i++ ) {
    if ( server.hasArg( GrowBox::fetName[i] ) ) {
      t = atoi( server.arg( GrowBox::fetName[i]).c_str() );
      growBox.fetSet( i, t );
    }
  }
  
  argFloat( config.tempMin,   "TEMPMIN"  );
  argFloat( config.tempMax,   "TEMPMAX"  );
  argInt(   config.humidMax,  "HUMIDMAX" );
  argInt(   config.humidMin,  "HUMIDMIN" );
  argInt(   config.tz,        "tz"       );
  argInt(   config.dst,       "dst"      );
}

void handleBoot() {
  growBox.oled.clear();
  growBox.oled.println( "* * * REBOOT * * *" );
  server.sendHeader( "Location", String("/"), true );
  server.sendHeader( "Connection", "close" );
  server.send ( 302, "text/plain", "" );
  delay( 2500 );
  WiFi.disconnect();
  growBox.oled.clear();
  ESP.restart();
}

void handleRoot() {
  char temp[120];
  handleIO();
  snprintf_P( temp, sizeof( temp ),
    PSTR("<html><head><script src='https://bjork.es/js/growbox.js?l=%d'></script></head><body></body></html>"),
    millis()
  );
  server.sendHeader( "Cache-Control", "public, max-age=86400" );
  server.sendHeader( "Connection", "close" );
  server.send ( 200, "text/html", temp );
}

void handleJSON() {
  char temp[300];
  char buff[200] = "";
  
  handleIO();

  config.toJson( buff, 200 );
  strncpy( temp, "{\"config\":", sizeof( temp ) );
  strncat( temp, buff, sizeof( temp ) );
  strncat( temp, ",\"state\":", sizeof( temp ) );
  growBox.toJson( buff, 200 );
  strncat( temp, buff, sizeof( temp ) );
  strncat( temp, "}", sizeof( temp ) );
  
  server.sendHeader( "Cache-Control", "no-cache" );
  server.sendHeader( "Connection", "close" );
  server.send ( 200, "application/json", temp );
}

void initWifi(){
  uint8_t i = 0;
  
  WiFi.mode( WIFI_STA );
  WiFi.begin( config.ssid, config.pass );
  
  while ( WiFi.status() != WL_CONNECTED && i++ < 10 ) {
    delay( 500 );
    growBox.oled.setCursor( 0, 1 );
    growBox.oled.print( spinner[ i % sizeof( spinner ) ] );
  }
  
  growBox.oled.setCursor( 0, 0 );
  if ( WiFi.status() == WL_CONNECTED ) {
    if ( !MDNS.begin( config.name ) )
      delay( 500 );
    server.on( "/", handleRoot );
    server.on( "/js", handleJSON );
    server.on( "/boot", handleBoot );
    httpUpdater.setup( &server );
    server.begin();
    MDNS.addService("http", "tcp", 80);
  }
  growBox.oled.clear();
}

void setup(void){
  config.read( "/config.json" );
  growBox.init();
  growBox.oled.clear();
  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
    growBox.oled.println("Station disconnected");
    initWifi();
  });
  initWifi();
  setSyncInterval( 24*60*60 );    // Daily
  setSyncProvider( syncHTTP );
}

void loop(void){
  time_t t = now() + config.tz * SECS_PER_HOUR + ( checkDst() ? SECS_PER_HOUR : 0 );
  if ( hour( t ) == 6 && minute( t ) == 15 && growBox.fetStatus( 0 ) == 0 )
    growBox.fetSet( GrowBox::LED, 1 );
  if ( hour( t ) == 20 && minute( t ) == 00 && growBox.fetStatus( 0 ) > 0 )
    growBox.fetSet( GrowBox::LED, 0 );
  
  growBox.update();
  
  if ( WiFi.status() == WL_CONNECTED ) {
    server.handleClient();
  } else {
    initWifi();
  }
  
  growBox.oled.setCursor( 0, 0 );
  growBox.oled.printf( "%s%*s%02d:%02d:%02d ", 
    config.name, 13 - strlen( config.name ), "", // OLED width 21.5 char
    hour( t ), minute( t ), second( t ) );
  
  growBox.oled.setCursor( 0, 1 );
  if ( WiFi.status() == WL_CONNECTED ) {
    growBox.oled.print( WiFi.localIP() );
  } else {
    growBox.oled.println( "No WiFi " );
  }
  
  growBox.oled.setCursor( 0, 2 );
  growBox.oled.printf( "% 7.1fC% 7.1f%% ", growBox.temperature, growBox.humidity );
  
  growBox.oled.setCursor( 0, 3 );
//  growBox.oled.printf( "Led:% 4d   Fan:% 4d \n", growBox.fetStatus( 0 ), growBox.fetStatus( 1 ) );

//  wifi_set_sleep_type( LIGHT_SLEEP_T );
  yield();
}

