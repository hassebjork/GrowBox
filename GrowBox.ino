/* Strings
 *   https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
 * 
 * TODO
 * Logging 
 *   Save data to server 
 *   Save data to SPIFFS
 *   Download data file
 * Config
 *   Upload file (POST
 *   Download file (GET)
 */

#include <pgmspace.h>     // PROGMEM functions

#include "Config.h"                         // Configuration class for local storage
Config config;

#include "GrowBox.h"
GrowBox growBox;

// Time and sync
#include <TimeLib.h>      // https://github.com/PaulStoffregen/Time
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char * headerKeys[] = { "date" };
const size_t numberOfHeaders = 1;
const char * _months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }; 

// WiFiManager
#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
WiFiManager wifiManager;

//#include <pgmspace.h>                       // PSTR & PROGMEM
#include <ESP8266WiFi.h>
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
    return makeTime( tm );
  }
  if ( now() < 1000 )
    setTime( 0, 0, 0, 1, 1, 2000 );
  return now();
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

void handleIO() {
  int t;
  for ( uint8_t i = 0; i < GrowBox::fetNo; i++ ) {
    if ( server.hasArg( GrowBox::fetName[i] ) ) {
      t = atoi( server.arg( GrowBox::fetName[i] ).c_str() );
      growBox.fetSet( i, t );
    }
  }
  for ( uint8_t i = 0; i < Config::attrNo; i++ ) {
    if ( server.hasArg( Config::attr[i] ) ) {
      config.set( i, server.arg( Config::attr[i] ).c_str() );
    }
  }
  config.save();
}

void handleBoot() {
  growBox.oled.clear();
  growBox.oled.println( "* * * REBOOT * * *" );
  server.sendHeader( "Location", "/" );
  server.send ( 303 );
  delay( 2500 );
  WiFi.disconnect();
  growBox.oled.clear();
  ESP.restart();
}

void handleRoot() {
  handleIO();
  char temp[120];
  snprintf_P( temp, sizeof( temp ),
    PSTR("<html><head><script src='https://bjork.es/js/growbox.js?l=%d'></script></head><body></body></html>"),
    millis()
  );
//  const char temp[] = "<html><head><script src='https://bjork.es/js/growbox.js'></script></head><body></body></html>";
  server.sendHeader( "Cache-Control", "public, max-age=86400" );
  server.sendHeader( "Connection", "close" );
  server.send ( 200, "text/html", temp );
}

void handleJSON() {
  char temp[350];
  char buff[250] = "";
  handleIO();

  strncpy( temp, "{\"chip\":", sizeof( temp ) );
  itoa( ESP.getChipId(), buff, 10 );
  strncat( temp, buff, sizeof( temp ) );
  strncat( temp, ",\"config\":", sizeof( temp ) );
  config.toJson( buff, sizeof( buff ) );
  strncat( temp, buff, sizeof( temp ) );
  strncat( temp, ",\"state\":", sizeof( temp ) );
  growBox.toJson( buff, sizeof( buff ) );
  strncat( temp, buff, sizeof( temp ) );
  strncat( temp, "}", sizeof( temp ) );
  
  server.sendHeader( "Cache-Control", "no-cache" );
  server.sendHeader( "Connection", "close" );
  server.send ( 200, "application/json", temp );
  
//  DEBUG_MSG("handleJSON: %d bytes\n", strlen( temp ) );  
}

void wifiAPMode( WiFiManager *wm ){
  growBox.oled.clear();
  growBox.oled.setCursor( 0, 0 );
  growBox.oled.println( F( "No network found" ) );
  growBox.oled.print( F( "Connect to AP:\nhttp://" ) );
  growBox.oled.println( WiFi.softAPIP() ); 
  growBox.oled.print( wm->getConfigPortalSSID() ); 
  growBox.oled.println( F( ".local" ) );
}

void initWifi(){
  growBox.oled.print( F( "WiFi: Connecting" ) );
  wifiManager.autoConnect( config.name );
  
  if ( WiFi.status() == WL_CONNECTED ) {
    if ( !MDNS.begin( config.name ) )
      delay( 500 );
    server.on( "/", handleRoot );
    server.on( "/js", handleJSON );
    server.on( "/boot", handleBoot );
    httpUpdater.setup( &server );
    server.begin();
    MDNS.addService("http", "tcp", 80);
    setSyncInterval( 24*60*60 );    // Daily
    setSyncProvider( syncHTTP );
  }
  growBox.oled.clear();
}

void setup(void){
  Serial.begin(115200);
  config.load();
  growBox.oled.clear();
  wifiManager.setTimeout( 300 );
  wifiManager.setAPCallback( wifiAPMode );
//  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
//    growBox.oled.println("Station disconnected");
//    initWifi();
//  });
  initWifi();
}

void loop(void){
  config.time = now() + config.tz * SECS_PER_HOUR + ( checkDst() ? SECS_PER_HOUR : 0 );

  if ( second( config.time ) == 0 ) {
    if ( hour( config.time ) == config.ledOn.hour && minute( config.time ) == config.ledOn.minute )
      growBox.fetSet( GrowBox::LED, GrowBox::PWM_MAX );
    if ( hour( config.time ) == config.ledOff.hour && minute( config.time ) == config.ledOff.minute )
      growBox.fetSet( GrowBox::LED, 0 );
  }
  
  growBox.update();
  
  growBox.oled.setCursor( 0, 0 );
  growBox.oled.printf( "%s%*s%02d:%02d:%02d ", 
    config.name, 13 - strlen( config.name ), "", // OLED width 21.5 char
    hour( config.time ), minute( config.time ), second( config.time ) );
  
  growBox.oled.setCursor( 0, 1 );
  if ( WiFi.status() == WL_CONNECTED ) {
    growBox.oled.print( WiFi.localIP() );
    server.handleClient();
  } else {
    growBox.oled.println( F( "No WiFi ") );
    wifiManager.setTimeout( 10 );
    initWifi();
  }
  
  growBox.oled.setCursor( 0, 2 );
  growBox.oled.printf( "% 7.1fC% 7.1f%% ", growBox.temperature, growBox.humidity );
  
  growBox.oled.setCursor( 0, 3 );

//  wifi_set_sleep_type( LIGHT_SLEEP_T );
  yield();
}
