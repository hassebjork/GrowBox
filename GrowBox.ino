/* Strings
 *   https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/
 * 
 * TODO
 * Security
 *   Password lock handleIO, fw update and file uploads
 * 
 * Logging 
 *   Save data to server 
 * 
 * Integration
 *   iframe to database with growbox contents
 */

#include <pgmspace.h>           // PROGMEM functions
#include <string.h>             // strncat etc
#include <pgmspace.h>           // PSTR & PROGMEM
#include "FS.h"                 // https://github.com/esp8266/Arduino/tree/master/cores/esp8266
File file;

#include "Config.h"             // Configuration class for local storage
Config config;

#include "GrowBox.h"
GrowBox growBox;

// Time and sync
#include <TimeLib.h>      // https://github.com/PaulStoffregen/Time
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char CONTENT_TYPE_TXT[]  = "text/plain";
const char CONTENT_TYPE_HTM[]  = "text/html";
const char CONTENT_TYPE_CSS[]  = "text/css";
const char CONTENT_TYPE_JS[]   = "application/javascript";
const char CONTENT_TYPE_JSON[] = "application/json";
const char CONTENT_TYPE_PNG[]  = "image/png";
const char CONTENT_TYPE_GIF[]  = "image/gif";
const char CONTENT_TYPE_JPG[]  = "image/jpeg";
const char CONTENT_TYPE_ICO[]  = "image/x-icon";
const char CONTENT_TYPE_XML[]  = "text/xml";
const char CONTENT_TYPE_PDF[]  = "application/x-pdf";
const char CONTENT_TYPE_ZIP[]  = "application/x-zip";
const char CONTENT_TYPE_GZ[]   = "application/x-gzip";
const char CONTENT_TYPE_DAT[]  = "application/octet-stream";

const char HEADER_CONT_ENC[]   = "Content-Encoding";
const char HEADER_ACCESS_CTR[] = "Access-Control-Allow-Origin";
const char HEADER_CACHE[]      = "Cache-Control";
const char HEADER_CONNECTION[] = "Connection";
const char HEADER_CLOSE[]      = "close";
const char HEADER_LOCATION[]   = "Location";

const char FTYP_GZ[]             = ".gz";

const char FILE_FILE[]         = "file";
const char FILE_DIR[]          = "dir";
const char FILE_DEL[]          = "del";
const char FILE_FAVICON[]      = "/favicon.ico";
const char FILE_NOT_CREATE[]   = "500: Couldn't create file";
const char FILE_NOT_FOUND[]    = "404: Not Found";

const char * headerKeys[]      = { "date" };
const size_t numberOfHeaders   = 1;

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

time_t syncHTTP() {
  tmElements_t tm;
  if ( WiFi.status() == WL_CONNECTED ) {
    HTTPClient http;
    http.begin( F( "http://google.com/" ) ); 
    http.collectHeaders( headerKeys, numberOfHeaders );
    
    if ( http.GET() > 0 ) {
      uint8_t i;
      String headerDate = http.header( headerKeys[0] );
      const char * str  = headerDate.c_str();
	  
      for ( i = 0; i < 12; i++ ) {
        if ( strncmp_P( Config::months[i], str + 8, 3 ) == 0 )
          break;
      }
      
      tm.Year   = ( str[14] - '0' ) * 10 + ( str[15] - '0' ) + 30;
      tm.Month  = i + 1;
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

void handleIO() {
  for ( uint8_t i = 0; i < GrowBox::fetNo; i++ ) {
    if ( server.hasArg( GrowBox::fetName[i] ) ) {
      growBox.fetSet( i, atoi( server.arg( GrowBox::fetName[i] ).c_str() ) );
    }
  }
  for ( uint8_t i = Config::NAME; i < Config::UPDATETIME; i++ ) {
    if ( server.hasArg( Config::attr[i] ) ) {
      config.set( i, server.arg( Config::attr[i] ).c_str() );
    }
  }
  config.save();
}

void handleBoot() {
  growBox.oled.clear();
  growBox.oled.println( F( "* * * REBOOT * * *" ) );
  server.send ( 200, CONTENT_TYPE_HTM, F( "<html><head><meta http-equiv='refresh' content='5;url=/'/></head></html>" ) );
  DEBUG_MSG( "handleBoot: Rebooting\n" );  
  delay( 500 );
  WiFi.disconnect();
  ESP.restart();
}

void handleJSON() {
  char temp[350];
  handleIO();

  strncpy( temp, "{", sizeof( temp ) );
  Config::jsonAttribute( temp, "ver", false, sizeof( temp ) );
  Config::toJson( temp, __DATE__, sizeof( temp ) );
  Config::jsonAttribute( temp, "chip", true, sizeof( temp ) );
  Config::toJson( temp, (int)ESP.getChipId(), sizeof( temp ) );
  Config::jsonAttribute( temp, "config", true, sizeof( temp ) );
  config.toJson( temp, sizeof( temp ) );
  Config::jsonAttribute( temp, "state", true, sizeof( temp ) );
  growBox.toJson( temp, sizeof( temp ) );
  strncat( temp, "}", sizeof( temp ) );
  server.sendHeader( HEADER_ACCESS_CTR, "*" );
  server.sendHeader( HEADER_CACHE, "no-cache" );
  server.sendHeader( HEADER_CONNECTION, HEADER_CLOSE );
  server.send ( 200, CONTENT_TYPE_JSON, temp );
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

const char* getContentType( String filename ) {
  if ( filename.endsWith( ".html" ) ) return CONTENT_TYPE_HTM;
  else if ( filename.endsWith( ".htm"  ) ) return CONTENT_TYPE_HTM;
  else if ( filename.endsWith( ".css"  ) ) return CONTENT_TYPE_CSS;
  else if ( filename.endsWith( ".js"   ) ) return CONTENT_TYPE_JS;
  else if ( filename.endsWith( ".json" ) ) return CONTENT_TYPE_JSON;
  else if ( filename.endsWith( ".png"  ) ) return CONTENT_TYPE_PNG;
  else if ( filename.endsWith( ".gif"  ) ) return CONTENT_TYPE_GIF;
  else if ( filename.endsWith( ".jpg"  ) ) return CONTENT_TYPE_JPG;
  else if ( filename.endsWith( ".ico"  ) ) return CONTENT_TYPE_ICO;
  else if ( filename.endsWith( ".xml"  ) ) return CONTENT_TYPE_XML;
  else if ( filename.endsWith( ".pdf"  ) ) return CONTENT_TYPE_PDF;
  else if ( filename.endsWith( ".zip"  ) ) return CONTENT_TYPE_ZIP;
  else if ( filename.endsWith( FTYP_GZ ) ) return CONTENT_TYPE_GZ;
  else if ( filename.endsWith( ".dat"  ) ) return CONTENT_TYPE_DAT;
  return CONTENT_TYPE_TXT;
}

bool fileDownload( String path ) {
  DEBUG_MSG( "fileDownload: %s\n", path.c_str() );
  if ( !path.startsWith( "/" ) )
    path = "/" + path;
  const char* contentType = getContentType( path );
  String pathWithGz = path + FTYP_GZ;
  if ( SPIFFS.exists( pathWithGz ) || SPIFFS.exists( path ) ) {
    if ( SPIFFS.exists( pathWithGz ) ) {
      path += FTYP_GZ;
      server.sendHeader( HEADER_CONT_ENC, "gzip" );
    }
    server.sendHeader( HEADER_ACCESS_CTR, "*" );
    File file = SPIFFS.open( path, "r" );
    size_t sent = server.streamFile( file, contentType );
    file.close();
    DEBUG_MSG( "\tSent file: %s\n", path.c_str() );
    return true;
  }
  DEBUG_MSG("\tFile Not Found: %s\n", path.c_str() );
  return false;
}

void fileUpload() {
  HTTPUpload& upload = server.upload();
  if ( upload.status == UPLOAD_FILE_START ) {
    String filename = upload.filename;
    if ( !filename.startsWith( "/" ) ) filename = "/" + filename;
    DEBUG_MSG( "fileUpload Name: %s\n", filename.c_str() );
    file = SPIFFS.open( filename, "w" );
    filename = String();
  } else if ( upload.status == UPLOAD_FILE_WRITE ) {
    if ( file )
      file.write( upload.buf, upload.currentSize );
  } else if ( upload.status == UPLOAD_FILE_END ) {
    if ( file ) {
      file.close();
      DEBUG_MSG( "fileUpload Size: %d bytes\n", upload.totalSize );
      server.sendHeader( HEADER_LOCATION, "/" );
      server.send( 303 );
    } else {
      server.send( 500, CONTENT_TYPE_TXT, FILE_NOT_CREATE );
    }
  }
}

void fileList( String path = "/" ) {
  DEBUG_MSG( "fileList: %s\n", path.c_str()  );
  Dir dir = SPIFFS.openDir(path);
  String output = "[";
  while ( dir.next() ) {
    File entry = dir.openFile( "r" );
    if ( output != "[" ) {
      output += ",\n";
    }
    output += "[\"";
    output += entry.name();
    output += "\",";
    output += String( entry.size() );
    output += "]";
    entry.close();
  }

  output += "]";
  server.sendHeader( HEADER_ACCESS_CTR, "*" );
  server.send(200, CONTENT_TYPE_JSON, output);
}

void fileDelete( String path = "/" ) {
  DEBUG_MSG( "fileDelete: %s\n", path.c_str() );
  if ( path == "/" )
    return server.send( 500, CONTENT_TYPE_TXT, "BAD PATH" );
  if ( !SPIFFS.exists( path ) )
    return server.send(404, CONTENT_TYPE_TXT, FILE_NOT_FOUND );
  SPIFFS.remove( path );
  server.send( 200, CONTENT_TYPE_TXT, "Deleted!");
}

void setup(void){
  Serial.begin(115200);
  SPIFFS.begin();
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
  
  server.on( "/", []{
    handleIO();
    if ( !fileDownload( "/index.htm" ) )
      server.send( 404, CONTENT_TYPE_TXT, FILE_NOT_FOUND );
  } );
  server.on( "/js", handleJSON );
  server.on( "/boot", handleBoot );
  server.on( "/file", HTTP_POST, [](){
    server.send( 200 ); }, fileUpload );
  server.on( "/file", HTTP_GET, []() {
    if ( server.hasArg( FILE_FILE ) && fileDownload( server.arg( FILE_FILE ) ) ) {
      return;
    } else if ( server.hasArg( FILE_DIR ) ) {
      return fileList( server.arg( FILE_DIR ) );
    } else if ( server.hasArg( FILE_DEL ) ) {
      return fileDelete( server.arg( FILE_DEL ) );
    }
    server.send( 404, CONTENT_TYPE_TXT, FILE_NOT_FOUND );
  });
  server.on( FILE_FAVICON, []() {
    if ( !fileDownload( FILE_FAVICON ) )
      server.send( 404, CONTENT_TYPE_TXT, FILE_NOT_FOUND );
  } );
  httpUpdater.setup( &server );
  initWifi();
  
  setSyncInterval( 24*60*60 );    // Daily
  if ( WiFi.status() == WL_CONNECTED ) {
    setSyncProvider( syncHTTP );
  }
}

void loop(void){
  config.timeRefresh();
  
  if ( millis() < 5000 || second( config.time ) == 0 ) {
    if ( ( hour( config.time ) > config.ledOn.hour 
		|| ( hour( config.time ) == config.ledOn.hour 
		&& minute( config.time ) >= config.ledOn.minute ) )
		&& ( hour( config.time ) < config.ledOff.hour 
		|| ( hour( config.time ) == config.ledOff.hour 
		&& minute( config.time ) < config.ledOff.minute ) ) )
      growBox.fetSet( GrowBox::LED, GrowBox::PWM_MAX );
    else
      growBox.fetSet( GrowBox::LED, 0 );
  }
//   if ( second( config.time ) == 0 ) {
//     if ( hour( config.time ) == config.ledOn.hour && minute( config.time ) == config.ledOn.minute )
//       growBox.fetSet( GrowBox::LED, GrowBox::PWM_MAX );
//     if ( hour( config.time ) == config.ledOff.hour && minute( config.time ) == config.ledOff.minute )
//       growBox.fetSet( GrowBox::LED, 0 );
//   }
  
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
//  wifi_set_sleep_type( LIGHT_SLEEP_T );
  delay( 400 );
}
