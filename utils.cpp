#include "utils.h"

// output to console including timestamp
void log(String message){
    long ts = millis();
    Serial.print(String(ts) + " ");
    Serial.print(message);
}

void logln(String message){
    log(message + "\n");
}


// setup serial for logging
void init_serial(){
    Serial.begin(115200);
    while(!Serial) { }
    logln("\nConsole initiated");
}


// handle wifi connection
// speed up measures taken from https://pokewithastick.net/esp8266-fast-wifi-connect-post
#define GATEWAY IPAddress(192, 168, 1, 1)
#define MASK IPAddress(255, 255, 255, 0)
uint8_t home_mac[6] = { 0x38, 0xD5, 0x47, 0x84, 0x77, 0x00 };
int channel = 9;

void wifi_reconnect(const char* ssid, const char* wifi_password, byte ip_last_byte){
  // fast (re)connect
  int counter = 0;
  WiFi.config(IPAddress(192, 168, 1, ip_last_byte), GATEWAY, MASK);
  logln("Connecting to WiFi " + String(ssid));
  while (WiFi.status() != WL_CONNECTED) {
      delay(5);
      if (++counter > 1000) break;
  }

  // slow full connect
  if (WiFi.status() != WL_CONNECTED) {
      logln("Fast reconnect failed, doing full connect");
      if (!WiFi.getAutoConnect()) WiFi.setAutoConnect(true);
      if (!WiFi.getPersistent()) WiFi.persistent(true);
      WiFi.begin(ssid, wifi_password, channel, home_mac, true);
      counter = 0;
      while (WiFi.status() != WL_CONNECTED) {
          delay(200);
          logln(".");
          if (++counter > 20) {
              logln("wifi timed-out. Rebooting..");
              delay(100);
              ESP.restart();
          }
      }
  }
  logln("WiFi connected: " + WiFi.localIP().toString() + " / RSSI " + WiFi.RSSI());
}

void wifi_sleep(){
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  // need to return to wifi thread for previous command to take effect
  delay(1);
}

// revert sleep, needs full reconnect afterwards
void wifi_wakeup(){
  WiFi.forceSleepWake();
  delay(1);
  WiFi.mode(WIFI_STA);
}

// HTTP based communication with sensor_broker middleware
Broker::Broker(String broker_url) {
    _url = broker_url;
    _properties = 0;
    this->addProperty("RSSI", String(WiFi.RSSI()));
}

void Broker::addProperty(String name, String value) {
    if (_properties == 0){
        _url += "?";
    } else {
        _url += "&";
    }
    _url += name + "=" + value;
    _properties++;
}

bool Broker::upload(){
   HTTPClient http;
   logln("Uploading to " + _url);

   http.begin(_url.c_str());
   int response = http.GET();
   http.end();
   if (response > 0) {
     log("HTTP Response code: ");
     logln(String(response));
     return true;
   } else {
     log("Error code: ");
     logln(String(response));
     return false;
   }
}

String Broker::getUrl(){
    return _url;
}
