bool loadConfig() {
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("Failed to open config file");
        return false;
    }

    StaticJsonDocument<512> doc;
    auto error = deserializeJson(doc, configFile);
    if (error) {
        Serial.println("Failed to parse config file");
        return false;
    }

    strlcpy(config.host, doc["host"] | "haus_75", sizeof(config.host));    
    strlcpy(config.ssid, doc["ssid"] | "devnet-34", sizeof(config.ssid));  
    strlcpy(config.wifipassword, doc["wifipass"] | "testerwlan", sizeof(config.wifipassword));  
    strlcpy(config.adminuser, doc["adminuser"] | "admin", sizeof(config.adminuser));  
    strlcpy(config.adminpassword, doc["adminpass"] | "admin", sizeof(config.adminpassword));  
    strlcpy(config.webuser, doc["webuser"] | "admin", sizeof(config.webuser));  
    strlcpy(config.webpassword, doc["webpass"] | "admin", sizeof(config.webpassword));  
    strlcpy(config.mqttserver,doc["mqttserver"] | "192.168.12.2", sizeof(config.mqttserver));
    config.webserverport = doc["port"] | 80;

    // Real world application would store these values in some variables for
    // later use.

    configFile.close();
    return true;
}

bool saveConfig() {
    StaticJsonDocument<512> doc;

    doc["ssid"] = config.ssid;
    doc["host"] = config.host;
    doc["wifipass"] = config.wifipassword;
    doc["adminuser"] = config.adminuser;
    doc["adminpass"] = config.adminpassword;
    doc["webuser"] = config.webuser;
    doc["webpass"] = config.webpassword;
    doc["mqttserver"] = config.mqttserver;
    doc["port"] = config.webserverport;

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return false;
    }

    // Serialize JSON to file
    if (serializeJson(doc, configFile) == 0) {
        Serial.println(F("Failed to write to file"));
    }

    // Close the file
    configFile.close();
    return true;
}


void getSensorReadings() {
  //temperature =  sht1x.readTemperatureC();
  //humidity =  sht1x.readHumidity();
  #if defined(ESP8266)
    voltage  = ESP.getVcc() / 1023.0F;
  #endif
  //dewpoint = (243.5*(log(humidity/100)+((17.67*temperature)/(243.5+temperature)))/(17.67-log(humidity/100)-((17.67*temperature)/(243.5+temperature))));
}

String processor(const String& var) {
  Serial.println(var);
  if (var == "TEMPERATURE") {
    return String(temperature);
  }
  else if (var == "DEWPOINT") {
    return String(dewpoint);
  }
  else if (var == "HUMIDITY") {
    return String(humidity);
  }
  //---
  else if (var == "VOLTAGE") {
    return String(voltage);
  }
  else if (var == "BUILD_TIMESTAMP"){
    return String(__DATE__) + " " + String(__TIME__);
  }
  else if (var == "IP"){
    return WiFi.localIP().toString();
  }
  //----
  else
    return "?";
}


void initServer(){
    // Handle Web Server
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html").setTemplateProcessor(processor);
    // Server send JSON-DATA
    server.on("/api/sensor", HTTP_GET, [](AsyncWebServerRequest *request) {
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      DynamicJsonDocument json(1024);
      json["temperatur"] = String(temperature);
      json["feuchte"] = String(humidity);
      json["taupunkt"] = String(dewpoint);
      json["batterie"] = String(voltage);
      serializeJson(json, *response);
      request->send(response);
    });
    server.on("/api/wifi-info", HTTP_GET, [](AsyncWebServerRequest *request) {
      AsyncResponseStream *response = request->beginResponseStream("application/json");
      DynamicJsonDocument json(1024);
      json["status"] = "ok";
      json["ssid"] = WiFi.SSID();
      json["ip"] = WiFi.localIP().toString();
      serializeJson(json, *response);
      request->send(response);
    });
    // Handle Web Server Events
    events.onConnect([](AsyncEventSourceClient * client) {
        if (client->lastId()) {
            Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
        }
        // send event with message "hello!", id current millis
        // and set reconnect delay to 1 second
        client->send("hello!", NULL, millis(), 10000);
    });
    server.addHandler(&events);
    #ifdef ESP32
        server.addHandler(new FSEditor(LittleFS, "admin","admin"));
    #elif defined(ESP8266)
        server.addHandler(new FSEditor("admin","admin"));
    #endif


    /// MQTT Server Verbindung
    //client.setServer(config.mqttserver,1883);
    // Start AsyncElegantOTA
    AsyncElegantOTA.begin(&server);    
    // Start Server
    server.begin();

}



// Initialize WiFi
bool initWiFi() {
  unsigned long timer;
  WiFi.setHostname(config.host);
  WiFi.mode(WIFI_STA);
  int wifi_timeout = 10; // Sekunden
  timer = millis() / 1000;
  WiFi.begin(config.ssid, config.wifipassword);
  Serial.printf("Connecting to WiFi .. %s ",config.ssid);
  
  while (WiFi.status() != WL_CONNECTED && (millis() / 1000) < timer + wifi_timeout) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("\nTimeout beim Verbindungsaufbau");
    return false;
  } else {
    Serial.print("\nIP Address: ");
    Serial.println(WiFi.localIP());
    return true;
  }
}

void initPortal(){
  WiFi.softAP("Sensor-WiFi-Manager");
  server.on("/", HTTP_GET,[](AsyncWebServerRequest *request){
    request->send(LittleFS,"/wifimanager.html","text/html");
  });
  server.serveStatic("/", LittleFS, "/");
  server.begin();
}



void mqtt_reconnect(){
  while (!client.connected()){
    Serial.println("starte MQTT Verbindung....");
    String clientId = (config.host);
    clientId += "-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())){
      Serial.println("verbunden");
      client.publish(config.host , "online");
    } else {
      Serial.print("failed, rc= ");
      Serial.println(client.state());
      delay(5000);
    }




  }

}

void setupsensor() {
  Serial.println(F("BMP280 test"));
  unsigned status;
  //status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
  status = sensor.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(sensor.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }
   /* Default settings from datasheet. */
  sensor.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                     Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                     Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                     Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                     Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  Serial.println(F("Sensor online...."));
}

