#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>          
#endif

//needed for library
#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h>       //https://github.com/tzapu/WiFiManager

#include "AdafruitIO_WiFi.h"
// config.h must define the macros IO_USERNAME, IO_KEY and ADAFRUIT_IO_FEED
// respectively your Adafruit IO username, key and feed name
#include "config.h"


// select which pin will trigger the configuration portal when set to LOW
// ESP-01 users please note: the only pins available (0 and 2), are shared 
// with the bootloader, so always set them HIGH at power-up
#define SWITCH_PIN 5
#define WIFI_PIN 4
#define ZC_PIN 19
#define LDR_PIN 35 //ADC1_CH7
#define DIMMER_PIN 18

const unsigned long msInterval = 2000; // send light info once every msInterval milliseconds

unsigned long lastMeasure = 0;
int push_count = 0;
int dimmer_value = 0;
bool toggle = false;
bool configure_wifi = false;

AdafruitIO_WiFi* io = NULL;
AdafruitIO_Feed* dimmer_feed = NULL;
AdafruitIO_Feed* ldr_feed = NULL;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("\n Starting");

  pinMode(SWITCH_PIN, INPUT);
  pinMode(WIFI_PIN, INPUT);
  pinMode(ZC_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(DIMMER_PIN, OUTPUT);
//  configureWifi();
}

// this function is called whenever a feed message
// is received from Adafruit IO. it was attached to
// the feed in the loop() function below.
void handleMessage(AdafruitIO_Data *data) {
  Serial.println(data->toInt());
  //dimmer_value = data->toInt();
  //digitalWrite(DIMMER_PIN, dimmer_value);
}

void configureWifi() {
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //reset settings - for testing
  //wifiManager.resetSettings();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //it starts an access point with the specified name
  //here  "wifi_switch"
  //and goes into a blocking loop awaiting configuration

  //WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
  //WiFi.mode(WIFI_STA);
  
  if (!wifiManager.startConfigPortal("wifi_switch")) {
    Serial.println("failed to connect and hit timeout");
  }
  if (WiFi.status() == WL_CONNECTED){
    Serial.print("Connected to: ");
    char ssid[100];
    char password[100];
    strcpy(ssid, WiFi.SSID().c_str());
    strcpy(password, WiFi.psk().c_str());
    Serial.println(ssid);
    Serial.println(password);
    WiFi.disconnect();

    //connect to adafruitio
    if(io) delete io;
    Serial.println("Connecting to AdafruitIO");
    io = new AdafruitIO_WiFi(IO_USERNAME, IO_KEY, ssid, password);
    io->connect();
    dimmer_feed = io->feed(ADAFRUIT_IO_DIMMER_FEED);
    dimmer_feed->onMessage(handleMessage);
    ldr_feed = io->feed(ADAFRUIT_IO_LDR_FEED);
    // wait for a connection
    while(io->status() < AIO_CONNECTED) {
      Serial.print(".");
      delay(500);
    }
    Serial.println();
    Serial.println(io->statusText());
    dimmer_feed->get();
    ldr_feed->get();
  }
}

void loop() {
  if (io) io->run();

  // control press time
  if (digitalRead(SWITCH_PIN) == HIGH) {
    push_count += 1;
    //Serial.println(push_count);
    configure_wifi = false;
    toggle = false;
  } else {
    if (push_count > 0) {
      if (push_count > 3000) {
        configure_wifi = true;
      } else {
        toggle = true;
      }
    }
    push_count = 0;
  }
  
  if (digitalRead(WIFI_PIN) == HIGH) {
    configureWifi();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    if (millis() - lastMeasure > msInterval) {
      int light = analogRead(LDR_PIN);
      Serial.println(light);
      ldr_feed->save(light);
      lastMeasure = millis();
    }
  }
}
