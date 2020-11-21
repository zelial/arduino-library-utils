#include <ESP8266WiFi.h>
#include <string.h>
#include <ESP8266HTTPClient.h>
#include <Arduino.h>

// output to console including timestamp
void log(String message);
void logln(String message);


// setup serial for logging
void init_serial();

void wifi_reconnect(const char* ssid, const char* wifi_password, byte ip_last_byte);
void wifi_sleep();
void wifi_wakeup();

// HTTP based communication with sensor_broker middleware
class Broker {
  private:
    String _url;
    int _properties;
  public:
    Broker(String broker_url);
    void addProperty(String name, String value);
    bool upload();
    String getUrl();
};

