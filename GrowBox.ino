/* Strings
 *   https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
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
extern "C" {
  #include "user_interface.h"
}
ESP8266WebServer httpServer(80);

const char spinner[] = ".oOo";

/* TODO: Add limits */
void argFloat( float &f, const char *s ) {
  if ( httpServer.hasArg( s ) )
    f = atof( httpServer.arg( s ).c_str() );
}
void argInt( uint8_t &f, const char *s ) {
  if ( httpServer.hasArg( s ) )
    f = atoi( httpServer.arg( s ).c_str() );
}
void argInt( int8_t &f, const char *s ) {
  if ( httpServer.hasArg( s ) )
    f = atoi( httpServer.arg( s ).c_str() );
}

void handleIO() {
  for ( uint8_t i = 0; i < GrowBox::fetNo; i++ ) {
    if ( httpServer.hasArg( GrowBox::fetName[i] ) )
      growBox.fetSet( i, atoi( httpServer.arg( GrowBox::fetName[i]).c_str() ) );
  }
  
  argFloat( config.tempMin,   "TEMPMIN"  );
  argFloat( config.tempMax,   "TEMPMAX"  );
  argInt(   config.humidMax,  "HUMIDMAX" );
  argInt(   config.humidMin,  "HUMIDMIN" );
  argInt(   config.tz,        "tz"       );
  argInt(   config.dst,       "dst"      );
}

void handleRoot() {
  char temp[500];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  char buff[120] = "";
  
  handleIO();

  for ( uint8_t i = 0; i < GrowBox::fetNo; i++ ) {
    strncat( buff, "<a href=\"?", sizeof( buff ) );
    strncat( buff, GrowBox::fetName[i], sizeof( buff ) );
    strncat( buff, "=", sizeof( buff ) );
    strncat( buff, (growBox.fetStatus(i)?"0":"1"), sizeof( buff ) );
    strncat( buff, "\">", sizeof( buff ) );
    strncat( buff, GrowBox::fetName[i], sizeof( buff ) );
    strncat( buff, "</a> ", sizeof( buff ) );
  }

  snprintf_P( temp, sizeof( temp ),
PSTR("<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>%s</title>\
    <link rel='stylesheet' href='http://bjork.es/handheld.css' type='text/css' />\
  </head>\
  <body>\
    <h1>%s</h1>\
    <div><span>Uptime:</span> %02d:%02d:%02d</div>\
    <div><span>Temperature:</span> %.1f</div>\
    <div><span>Humidity:</span> %.1f</div>\
    <div>%s</div>\
  </body>\
</html>"),
    config.name, config.name, hr, min % 60, sec % 60,
    growBox.temperature, growBox.humidity,
    buff
  );
  httpServer.send ( 200, "text/html", temp );
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
  
  httpServer.send ( 200, "application/json", temp );
}

void initWifi(){
  uint8_t i = 0;
  
  WiFi.mode( WIFI_STA );
  WiFi.begin( config.ssid, config.pass );
  
  while ( WiFi.status() != WL_CONNECTED && i++ < 100 ) {
    delay( 500 );
    growBox.oled.setCursor( 0, 1 );
    growBox.oled.print( spinner[ i % sizeof( spinner ) ] );
  }
  
  growBox.oled.setCursor( 0, 0 );
  if ( WiFi.status() == WL_CONNECTED ) {
    growBox.oled.print( WiFi.localIP() );
    growBox.oled.print( " WiFi " );
    
    if ( MDNS.begin( config.name ) ) {
      growBox.oled.print( "*  " );
    }
    httpServer.on( "/", handleRoot );
    httpServer.on( "/js", handleJSON );
    httpServer.begin();
    MDNS.addService("http", "tcp", 80);
  } else {
    growBox.oled.print( "No WiFi     " ); 
  }
  
  growBox.oled.println( config.name );
}

void setup(void){
  config.read( "/config.json" );
  growBox.init();
  growBox.oled.clear();
  initWifi();
//  gtime.init( config.tz, config.dst );  
}

void loop(void){
  growBox.update();
  
  if ( WiFi.status() == WL_CONNECTED ) {
    httpServer.handleClient();
//  } else {
//    initWifi();
  }
  
  growBox.oled.setCursor( 0, 1 );
  growBox.oled.println( config.name );
  growBox.oled.print( "Temp:  " );
  growBox.oled.println( growBox.temperature, 1 );
  growBox.oled.print( "Humid: " );
  growBox.oled.println( growBox.humidity, 1 );
  growBox.oled.println( millis() );

//  wifi_set_sleep_type( LIGHT_SLEEP_T );
  delay( 900 );
}

