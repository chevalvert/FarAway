
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

bool wifi_available = false;
byte wifi_retry = 0;
byte wifi_otaEnable = false;
String wifi_nameDevice = "esp32";
void (*wifi_conClbck)();
byte wifi_maxRetry = 0;



/*
   Setup OTA
*/
void wifi_ota(String nameDevice) {
  wifi_otaEnable = true;
  wifi_nameDevice = nameDevice;
  ArduinoOTA.setHostname(nameDevice.c_str());
  _wifi_otabegin();
}
void _wifi_otabegin() {
  if (!wifi_otaEnable || !wifi_available) return;
  ArduinoOTA.begin();
  LOGINL("OTA: started = ");
  LOG(wifi_nameDevice);
}

/*
   Setup static WIFI
*/
void wifi_static(String ip, String gateway, String mask) {
  IPAddress addrIP;
  addrIP.fromString(ip);
  IPAddress gateIP;
  if (gateway == "auto") {
    gateIP.fromString(ip);
    gateIP[3] = 1;
  }
  else gateIP.fromString(gateway);
  IPAddress maskIP;
  maskIP.fromString(mask);
  WiFi.config(addrIP, gateIP, maskIP);
}
void wifi_static(String ip, String gateway) {
  wifi_static(ip, gateway, "255.255.255.0");
}
void wifi_static(String ip) {
  wifi_static(ip, "auto");
}


/*
   Connect as STATION
*/
void wifi_connect(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    if (wifi_available) LOG("WIFI: disconnected");
    wifi_available = false;
    _wifi_disconnected();
    wifi_retry += 1;

    // Check max retry
    if (wifi_maxRetry > 0 && wifi_retry > wifi_maxRetry) wifi_stop();
    else {
      LOGF("WIFI: reconnecting.. %i\n", wifi_retry);
      WiFi.reconnect();
    }
  }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    wifi_retry = 0;
    if (wifi_available) return;
    wifi_available = true;
    LOGINL("WIFI: connected = ");
    LOG(WiFi.localIP());
    _wifi_connected();
  }, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);

  WiFi.begin(ssid, password);
}

/*
 * Set Callback triggered when connection
 * is (re-)established
 */
void wifi_onConnect(void (*f)()) {
  wifi_conClbck = f;
}

/*
   Get connection status
*/
bool wifi_isok() {
  return wifi_available;
}

/*
   Internal callback
*/
void _wifi_connected() {
  _wifi_otabegin();
  //(*wifi_conClbck)();
}

/*
   Internal callback
*/
void _wifi_disconnected() {

}

/*
 * Wifi LOOP
 */
void wifi_loop() {
  // Run OTA
  if (wifi_otaEnable && wifi_available) ArduinoOTA.handle();
}

/*
   Wait for wifi to be connected, or until timeout
   Can trigger Restart on timeout
*/
void wifi_wait(int timeout, bool restart) {
  byte retries = 0;
  while (retries < timeout / 100) {
    if (wifi_available) return;
    retries += 1;
    delay(100);
  }
  if (restart) {
    LOG("\nWIFI: timeout is over, restarting..\n");
    ESP.restart();
  }
  else LOG("WIFI: timeout is over");
}

/*
   Disable wifi if no conn after x
*/
void wifi_maxTry(byte maxT) {
  wifi_maxRetry = maxT;
}

/*
   Disable wifi
*/
void wifi_stop() {
  LOG("Stopping wifi...");
  WiFi.mode(WIFI_OFF);
  btStop();
}