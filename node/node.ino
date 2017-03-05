/**
 * ESP Firmeware for sensor networ
 * 
 * This Software provides: 
 *  - A captive Portal to setup wifi connection
 *  - a preference set route to store: 
 *      - a new hostname
 *      - a post url 
 *  - a route to get the current date
 *  - a route to reset the eeprom
 *  - routes to set the sensor on and off
 *
 * Besides the /-route this programm post the current date to a given url
 * if its set every minute.
 *
 */
 
 
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// Controll LED
const int led = 5;

// Storage EEPROM Adress
int eepromNameAddress = 0;
byte value;

// Timer
os_timer_t myTimer;
bool tickOccured;

// POST
String remote_posthost;
String remote_postport;
String remote_posturl;

WiFiManager wifiManager;
WiFiClient client;
ESP8266WebServer server ( 80 );

// Captive Portal Callback
// =======================================================================================
void configModeCallback (WiFiManager *myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    //if you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());
}

// Timer Callback
// =======================================================================================
void timerCallback(void *pArg) {
    tickOccured = true;
}

// ROUTE to /
// =======================================================================================
void handleRoot() {
    digitalWrite ( led, 1 );
    char temp[400];
    int sec = millis() / 1000;
    int min = sec / 60;
    int hr = min / 60;

    snprintf ( temp, 400,
"{ \
  \"title\": \"%s\", \
  \"uptime\": \"%02d:%02d:%02d\", \
  \"mac\": \"%s\" \
}",
    WiFi.hostname().c_str(), hr, min % 60, sec % 60, WiFi.macAddress().c_str()
    );
    server.send ( 200, "application/json", temp );
    digitalWrite ( led, 0 );
}

// ROUTE to /set
// =======================================================================================
void setPreferences(){
    digitalWrite ( led, 1 );
    char temp[400];
    String hostname = server.arg("hostname");
    String posturl = server.arg("posturl");
    String posthost = server.arg("posthost");
    String postport = server.arg("postport");
    if( hostname.length() > 0 ){
        Serial.printf("Setting new Hostname: %s\n", hostname.c_str());
        WiFi.hostname(hostname);
        for (int i = 0; i < hostname.length(); ++i) {
            EEPROM.write(32+i, hostname[i]);
        }
        EEPROM.write(32+hostname.length(), '\0');
        EEPROM.commit();      
    }
    if( posturl.length() > 0 ){
        Serial.printf("Setting new posturl: %s\n", posturl.c_str());
        for (int i = 0; i < posturl.length(); ++i) {
            EEPROM.write(96+i, posturl[i]);
        }
        EEPROM.write(96+posturl.length(), '\0');
        EEPROM.commit();   
        remote_posturl = posturl + '\0';
    }    
    if( posthost.length() > 0 ){
        Serial.printf("Setting new posthost: %s\n", posthost.c_str());
        for (int i = 0; i < posthost.length(); ++i) {
            EEPROM.write(224+i, posthost[i]);
        }
        EEPROM.write(224+posthost.length(), '\0');
        EEPROM.commit();   
        remote_posthost = posthost + '\0';
    }    
    if( postport.length() > 0 ){
        Serial.printf("Setting new postport: %s\n", postport.c_str());
        for (int i = 0; i < postport.length(); ++i) {
            EEPROM.write(352+i, postport[i]);
        }
        EEPROM.write(352+postport.length(), '\0');
        EEPROM.commit();   
        remote_postport = postport + '\0';
    }        
    // Send preferences back
    snprintf ( temp, 400,
"{ \
  \"hostname\": \"%s\", \
  \"posturl\": \"%s\", \
  \"posthost\": \"%s\", \
  \"postport\": \"%s\" \
}",
    WiFi.hostname().c_str(), remote_posturl.c_str(), remote_posthost.c_str(), remote_postport.c_str()
    );
    server.send ( 200, "application/json", temp );
    digitalWrite ( led, 0 );
    // Reboot
    ESP.reset();
    delay(1000);
}
 
// 404
// =======================================================================================  
void handleNotFound() {
    digitalWrite ( led, 1 );
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for ( uint8_t i = 0; i < server.args(); i++ ) {
        message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
    }
    server.send ( 404, "text/plain", message );
    digitalWrite ( led, 0 );
}

// System Setup
// ======================================================================================= 
void setup() {
    Serial.begin(115200);
    pinMode ( led, OUTPUT );

    EEPROM.begin(4096);
  
    // Hostname
    String hostname = "";
    for (int i = 32; i < 96; ++i) {
        hostname += char(EEPROM.read(i));
    }
    Serial.print("EEPROM Hostname: ");
    Serial.println( hostname.c_str() );
    if(hostname.length() < 0){
        hostname = "grow";
    }
    WiFi.hostname(hostname.c_str());
    if ( MDNS.begin ( hostname.c_str() ) ) {
        Serial.println ( "MDNS responder started" );
    }  
    Serial.printf("Hostname: %s\n", WiFi.hostname().c_str());

    // Posturl 
    for (int i = 96; i < 224; ++i) {
        remote_posturl += char(EEPROM.read(i));
    }
    Serial.print("EEPROM Posturl: ");
    Serial.println( remote_posturl.c_str() );

    // Posthost
    for (int i = 224; i < 352; ++i) {
        remote_posthost += char(EEPROM.read(i));
    }
    Serial.print("EEPROM Posthost: ");
    Serial.println( remote_posthost.c_str() );

    // Postport
    for (int i = 352; i < 384; ++i) {
        remote_postport += char(EEPROM.read(i));
    }
    Serial.print("EEPROM Postport: ");
    Serial.println( remote_postport.c_str() );
        
    // set callback that gets called when connecting to previous WiFi fails, and enters 
    // Access Point mode
    wifiManager.setAPCallback(configModeCallback);

    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect()) {
        Serial.println("failed to connect and hit timeout");
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    } 

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    // Timer
    os_timer_setfn(&myTimer, timerCallback, NULL);
    os_timer_arm(&myTimer, 1000 * 60, true);

    // Routes
    server.on ( "/", handleRoot );
    
    server.on ( "/on", []() {
        digitalWrite ( led, 1 );
        server.send(200, "application/json", "{\"led\": true}");
    });
    
    server.on ( "/off", []() {
        digitalWrite ( led, 0 );
        server.send(200, "application/json", "{\"led\": false}");
    });
    
    server.on("/reset", []() {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 4096; ++i) { EEPROM.write(i, 0); Serial.print("x"); }
        Serial.println("clearing done.");
        EEPROM.commit();
        wifiManager.resetSettings();
        server.send(200, "application/json", "{\"reset\": true}");
        delay(1000);
        ESP.reset();
        delay(1000);
    }); 
    
    server.on ("/set", setPreferences );
    
    server.onNotFound ( handleNotFound );
    
    server.begin();
    Serial.println ( "HTTP server started" );
    tickOccured = true;
}

// MAINLOOP
// ======================================================================================= 
void loop() {
    server.handleClient();
    if (tickOccured == true) {
        digitalWrite ( led, 1 );
        char temp[400];
        int sec = millis() / 1000;
        int min = sec / 60;
        int hr = min / 60;
        snprintf ( temp, 400, "%02d:%02d:%02d", hr, min % 60, sec % 60);
         
        Serial.print("Tick Occurred: ");
        Serial.println(temp);

        // POST DATA
        if(remote_posturl.length() <= 0){
          Serial.println("Skipping post, because no url ist set");
        } else {
          int port = atoi(remote_postport.c_str());
          if (client.connect(remote_posthost.c_str(), port)) {
            Serial.print("Send data to: ");
            Serial.print(remote_posthost.c_str());
            Serial.print(" (");
            Serial.print(remote_postport.c_str());
            Serial.print(") ");
            Serial.println(remote_posturl.c_str());

            /// SEND DATA
            String data = "{\"test\": true}";
            client.print(String("POST ") + remote_posturl.c_str() + " HTTP/1.1\r\n" +
              "Host: " + remote_posthost.c_str() + "\r\n" +
              "Content-Type: application/json\r\n" +
              "Content-Length: " + data.length() + "\r\n" +
              data + "\n");
              
            //Delay
            delay(100);
            // Read all the lines of the reply from server and print them to Serial
            Serial.println("Response: \n");
            while(client.available()){
              String line = client.readStringUntil('\r');
              Serial.print(line);
            }
            Serial.println("");
            Serial.println("done.");
          } else {
            Serial.print("Can not connect to: ");
            Serial.print(remote_posturl.c_str());
            Serial.print(" (");
            Serial.print(remote_postport.c_str());
            Serial.println(")");
          }
        }
        digitalWrite ( led, 0 );
        tickOccured = false;
    }
    yield();
}

