/* Strings
 *   https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
 * 
 * Upload file to SPIFFS
 *   https://tttapa.github.io/ESP8266/Chap12%20-%20Uploading%20to%20Server.html
 */

#include "Config.h"       // Configuration class for local storage
Config config;

#include "GrowBox.h"
GrowBox growBox;

//#include "GTime.h"
//GTime gtime;

#include <pgmspace.h>     // PSTR & PROGMEM

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
      if ( t > 1023 ) 
        t = 1023;
      if ( t < 0 ) 
        t = 0;
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
  server.send ( 302, "text/plain", "" );
  delay( 2500 );
  WiFi.disconnect();
  growBox.oled.clear();
  ESP.restart();
}

void handleRoot() {
  char temp[300];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  char buff[120] = "";
  
  handleIO();

  snprintf_P( temp, sizeof( temp ),
    PSTR("<html><head><script src='https://bjork.es/js/growbox.js?l=%d'></script></head><body></body></html>"),
    millis()
  );
  server.sendHeader("Connection", "close");
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
  
  server.sendHeader("Connection", "close");
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
    MDNS.begin( config.name );
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
  
//  gtime.init( config.tz, config.dst );  
}

void loop(void){
  growBox.update();
  
  if ( WiFi.status() == WL_CONNECTED ) {
    server.handleClient();
  } else {
    initWifi();
  }
  
  growBox.oled.setCursor( 0, 0 );
  growBox.oled.printf( "%*s ", 10 + strlen( config.name ) / 2, config.name );
  
  growBox.oled.setCursor( 0, 1 );
  if ( WiFi.status() == WL_CONNECTED ) {
    growBox.oled.print( WiFi.localIP() );
    growBox.oled.println( " WiFi" );
  } else {
    growBox.oled.println( "No WiFi" );
  }
  
  growBox.oled.setCursor( 0, 2 );
  growBox.oled.printf( "% 7.1fC% 7.1f%% ", growBox.temperature, growBox.humidity );
  
  growBox.oled.setCursor( 0, 3 );
  growBox.oled.printf( "Led:% 4d   Fan:% 4d \n", growBox.fetStatus( 0 ), growBox.fetStatus( 1 ) );

//  wifi_set_sleep_type( LIGHT_SLEEP_T );
  yield();
}

