typedef struct WIFI_CONFIG {
  char ssid[20];
  char host[20];
  char wifipassword[20];
  char webuser[20];
  char webpassword[20];
  char adminuser[20];
  char adminpassword[20];
  char mqttserver[20];
  int webserverport;
} WIFI_CONFIG;

WIFI_CONFIG config;

// Timer variables
unsigned long lastTime = 0;
unsigned long lastMeas = 0;
unsigned long timerDelay = 10000;
unsigned long time_meas = 60000;
bool IsRebootRequired = false;


// variable holding BME readout
float temperature;
float humidity;
float dewpoint;
float voltage;
float temp_array[48];
float humi_array[48];

// dummy status
bool status;

