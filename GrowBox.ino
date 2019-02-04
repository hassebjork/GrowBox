/* Strings
 *   https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
 * 
 * TODO
 * Logging 
 *   Save data to server 
 *   Save data to SPIFFS
 *   Download data file
 */

#include <pgmspace.h>           // PROGMEM functions
#include <string.h>             // strncat etc
#include <pgmspace.h>           // PSTR & PROGMEM

#include "Config.h"             // Configuration class for local storage
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

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
extern "C" {
  #include "user_interface.h"
}
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiEventHandler gotIPEventHandler;         // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/generic-examples.html#event-driven-methods
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
  if ( millis() < 50000 )
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

void handleUpload() {
  File file;
  HTTPUpload& upload = server.upload();
  if ( upload.status == UPLOAD_FILE_START ) {
    String filename = upload.filename;
    if ( !filename.startsWith( "/" ) )
      filename = "/" + filename;
    file = SPIFFS.open( filename, "w" );
  } else if ( upload.status == UPLOAD_FILE_WRITE ) {
    if ( file )
      file.write( upload.buf, upload.currentSize ); // Write the received bytes to the file
  } else if ( upload.status == UPLOAD_FILE_END ) {
    if ( file ) {                                   // If the file was successfully created
      file.close();                                 // Close the file again
      server.sendHeader( "Location", "/" );         // Redirect the client to the success page
      server.send( 303 );
    } else {
      server.send( 500, "text/plain", F( "500: couldn't create file" ) );
    }
  }  
}

void handleDownload() {
  SPIFFS.begin();
  File file;
  if ( server.hasArg( "file" ) )
    file = SPIFFS.open( server.arg( "file" ), "r" );
  if ( !file )
    file = SPIFFS.open( Config::config_file, "r" );
  size_t sent = server.streamFile( file, "text/plain" );
  file.close();
}

void handleBoot() {
  growBox.oled.clear();
  growBox.oled.println( F( "* * * REBOOT * * *" ) );
  server.send ( 200, "text/html", F( "<html><head><meta http-equiv='refresh' content='5;url=/'/></head></html>" ) );
  DEBUG_MSG( "handleBoot: Rebooting\n" );  
  delay( 500 );
  WiFi.disconnect();
  ESP.restart();
}

void handleRoot() {
  handleIO();
  char temp[120];
  snprintf_P( temp, sizeof( temp ),
    PSTR("<html><head><script src='https://bjork.es/js/growbox.js?l=%d'></script></head><body></body></html>"),
    millis()
  );

  server.sendHeader( "Cache-Control", "public, max-age=86400" );
  server.sendHeader( "Connection", "close" );
  server.send ( 200, "text/html", temp );
//  server.send ( 200, "text/html", F("<html><head><script src='https://bjork.es/js/growbox.js'></script></head><body></body></html>") );
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
  growBox.oled.print( F( "Connect to AP:\n" ) );
  growBox.oled.println( wm->getConfigPortalSSID() ); 
  growBox.oled.print( F( "http://" ) );
  growBox.oled.println( WiFi.softAPIP() ); 
}

void initWifi(){
  growBox.oled.setCursor( 0, 1 );
  growBox.oled.print( F( "WiFi: Connecting" ) );
  growBox.oled.clearToEOL();
  wifiManager.autoConnect( config.name );
}

void setup(void){
  Serial.begin(115200);
  config.load();
  growBox.oled.clear();
  wifiManager.setTimeout( 300 );
  wifiManager.setAPCallback( wifiAPMode );
  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
    growBox.oled.setCursor( 0, 1 );
    growBox.oled.print( F("Disconnected") );
    growBox.oled.clearToEOL();
  });
  gotIPEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event) {
    growBox.oled.setCursor( 0, 1 );
    growBox.oled.print( WiFi.localIP() );
    growBox.oled.clearToEOL();
    MDNS.begin( config.name );
    MDNS.addService( "http", "tcp", 80 );
    server.begin();
  });
  server.on( "/", handleRoot );
  server.on( "/js", handleJSON );
  server.on( "/boot", handleBoot );
  server.on( "/dl", handleDownload );
  server.on( "/ul", handleUpload );
  httpUpdater.setup( &server );
  initWifi();
  
  setSyncInterval( 24*60*60 );    // Daily
  if ( WiFi.status() == WL_CONNECTED ) {
    setSyncProvider( syncHTTP );
  }
}

void loop(void){
  config.time = now() + config.tz * SECS_PER_HOUR + ( checkDst() ? SECS_PER_HOUR : 0 );

  if ( second( config.time ) == 0 ) {
    if ( hour( config.time ) == config.ledOn.hour && minute( config.time ) == config.ledOn.minute )
      growBox.fetSet( GrowBox::LED, GrowBox::PWM_MAX );
    if ( hour( config.time ) == config.ledOff.hour && minute( config.time ) == config.ledOff.minute )
      growBox.fetSet( GrowBox::LED, 0 );
  }
  
  if ( millis() - config.updMillis >= config.updateTime ) {
    config.updMillis += config.updateTime;
    growBox.update();
    
    // Update display
    growBox.oled.setCursor( 0, 0 );
    growBox.oled.printf( "%s%*s%02d:%02d:%02d ", 
      config.name, 13 - strlen( config.name ), "", // OLED width 21.5 char
      hour( config.time ), minute( config.time ), second( config.time ) );
    
    growBox.oled.setCursor( 0, 1 );
    if ( WiFi.status() == WL_CONNECTED ) {
      growBox.oled.print( WiFi.localIP() );
      server.handleClient();
      MDNS.update();
    } else {
      growBox.oled.print( F( "No WiFi") );
      growBox.oled.clearToEOL();
      wifiManager.setTimeout( 10 );
      initWifi();
    }
    growBox.oled.setCursor( 0, 2 );
    growBox.oled.printf( "% 7.1fC% 7.1f%% ", growBox.temperature, growBox.humidity );
    growBox.oled.setCursor( 0, 3 );
  }
  if ( config.logTime > 0 && millis() - config.logMillis >= config.logTime ) {
    growBox.logRecord();
  }

//  wifi_set_sleep_type( LIGHT_SLEEP_T );
  yield();
}
