#include <Arduino.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>
#include <FSEditor.h>
#include <AsyncElegantOTA.h>

#include <Ticker.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_BMP280.h>

const char* ssid = "devnet-34";
const char* password = "testerwlan";


#if defined(ESP8266)
  // Switch from ADC to Vcc readout.
  ADC_MODE(ADC_VCC);
#endif   
//
WiFiClient espClient;
PubSubClient client(espClient);
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create an Event Source on /events
AsyncEventSource events("/events");
// Create Sensor Object
Adafruit_BMP280 sensor; // I2C


#include <variables.h>
#include <function_subs.h>

/*-----------------------------Programm Setup---------------------------------------------*/

void setup(void) {
  Serial.begin(115200);
  delay(300);
  Serial.println("Mounting LittleFS ...");
	if (!LittleFS.begin()) {
		// SPIFFS will be configured on reboot
		Serial.println("ERROR: Cannot mount LittleFS, Rebooting");
		delay(1000);
		ESP.restart();
		}
  setupsensor();

  status = loadConfig();
  
  
  if (initWiFi()){
      initServer();
      client.setServer(config.mqttserver, 1883);

    } else {
      initPortal();
    };
    status = saveConfig();
}

void loop(void) {
  if ((millis() - lastTime) > timerDelay) {
  }
  if (!client.connected()){
    mqtt_reconnect();
  }
  if (IsRebootRequired) {
		Serial.println("Rebooting ESP32: "); 
		delay(1000); // give time for reboot page to load
		ESP.restart();
	}
  client.loop();
}