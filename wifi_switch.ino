#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include "AdafruitIO_WiFi.h"
// config.h must define the macros IO_USERNAME and IO_KEY
// respectively your Adafruit IO username and key
#include "config.h"


// select which pin will trigger the configuration portal when set to LOW
// ESP-01 users please note: the only pins available (0 and 2), are shared 
// with the bootloader, so always set them HIGH at power-up
#define BUTTON_PIN 1
#define RELAY_PIN 2

int push_count = 0;
int light_state = LOW;
bool toggle = false;
bool configure_wifi = false;

AdafruitIO_WiFi* io = NULL;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("\n Starting");

  pinMode(BUTTON_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
}

void submit_to_feed(const char* feed_name, int value) {
  AdafruitIO_Feed* feed = io->feed(feed_name);
  feed->save(value);
}

// this function is called whenever a feed message
// is received from Adafruit IO. it was attached to
// the feed in the loop() function below.
void handleMessage(AdafruitIO_Data *data) {
  light_state = data->toPinLevel();
  digitalWrite(RELAY_PIN, light_state); 
}

void loop() {
  // control press time
  if (digitalRead(BUTTON_PIN) == HIGH) {
    push_count += 1;
    configure_wifi = false;
    toggle = false;
  } else {
    if (push_count > 0) {
      if (push_count > 50) {
        configure_wifi = true;
      } else {
        toggle = true;
      }
    }
    push_count = 0;
  }
  

  if (io) {
    io->run();
  }
  
  if (toggle) {
    light_state = !light_state;
    digitalWrite(RELAY_PIN, light_state);
    submit_to_feed(ADAFRUIT_IO_FEED, light_state);
    toggle = false;
  }
  
  if (configure_wifi) {
    Serial.println("resetting");
    
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
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    if(io) delete io;
    io = new AdafruitIO_WiFi(IO_USERNAME, IO_KEY, WiFi.SSID().c_str(), WiFi.psk().c_str());
    AdafruitIO_Feed* feed = io->feed(ADAFRUIT_IO_FEED);
    feed->onMessage(handleMessage);
    
    configure_wifi = false;
  }
}
