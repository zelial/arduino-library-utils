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
#define GATEWAY IPAddress(10, 2, 0, 1)
#define MASK IPAddress(255, 255, 255, 0)
uint8_t home_mac[6] = { 0x34, 0x60, 0xF9, 0x4D, 0x21, 0xF1 };
//uint8_t home_mac[6] = { 0x9C, 0xA2, 0xF4, 0xF2, 0x31, 0x10 };
int channel = 8;

void wifi_reconnect(const char* ssid, const char* wifi_password, byte ip_last_byte){
  // fast (re)connect
  int counter = 0;
  logln("Connecting to WiFi " + String(ssid));
  // this doesn't work on the tp-link I use now, commenting out
  /*
  WiFi.config(IPAddress(10, 2, 0, ip_last_byte), GATEWAY, MASK);
  while (WiFi.status() != WL_CONNECTED) {
      delay(5);
      if (++counter > 1000) break;
  }
  */

  // slow full connect
  if (WiFi.status() != WL_CONNECTED) {
      logln("Fast reconnect failed/skipped, doing full connect");
      if (!WiFi.getAutoConnect()) WiFi.setAutoConnect(true);
      if (!WiFi.getPersistent()) WiFi.persistent(true);
      WiFi.begin(ssid, wifi_password, channel, home_mac, true);
      //WiFi.begin(ssid, wifi_password);
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
  WiFi.persistent(true);
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

// turn off (infinite deep sleep) if battery voltage low
void protect_battery(float voltage){
    if (voltage < LOW_VOLTAGE){
        logln("Battery voltage low! Going to deep sleep.");
        ESP.deepSleep(0);
    }
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
   WiFiClient client;
   HTTPClient http; //must be declared after WiFiClient for correct destruction order, because used by http.begin(client,...)
   logln("Uploading to " + _url);

   http.begin(client, _url.c_str());
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

// convert battery voltage to battery capacity percentage
// simple, imprecise linear conversion
int voltage2percentage(float voltage){
    // 4.2V is max ~ equal to 100%
    // 100 / (4.2 - 3) = 83.3
   return (voltage - LOW_VOLTAGE) * 83.3;
}

