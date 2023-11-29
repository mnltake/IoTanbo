// board XIAOesp32C3
#include "esp32_e220900t22s_jp_lib.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "pointdict.h"
// #define INFLUXDB_CLIENT_DEBUG_ENABLE
#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
// WiFi AP SSID
#define WIFI_SSID "***SSID***"
// WiFi password
#define WIFI_PASSWORD "***PASS***"

#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "***INFLUX API TOKEN ***"
#define INFLUXDB_ORG "***INFLUX ORG***"
#define INFLUXDB_BUCKET "lorasensor"

// Time zone info
#define TZ_INFO "UTC9"
  // Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Declare Data point
Point sensor("wifiGW");
#include <FS.h>

bool toInflux = false;
union ByteFloatUnion{
  uint8_t byteformat[4];
  float floatformat;
} temp;
uint16_t bootcount = 0;
uint16_t sensorID;
uint16_t water;
unsigned long t;
unsigned long resettime = 1000 * 60 *60 *24UL;
double lat ;
double lon ;
CLoRa lora;
struct LoRaConfigItem_t config = {
      0xFFFF, // own_address FFFF
      0b011, // baud_rate 9600 bps
      0b10000, // air_data_rate 1758bps SF:9 BW:125
      0b11, // subpacket_size 32byte
      0b1, // rssi_ambient_noise_flag 有効
      0b0, // transmission_pause_flag 有効
      0b01, // transmitting_power 13 dBm
      0x00, // own_channel 0
      0b1, // rssi_byte_flag 有効
      0b0, // transmission_method_type 
      0b0, // lbt_flag 有効
      0b011, // wor_cycle 2000 ms
      0x0000, // encryption_key 0
      0x0000, // target_address 0
      0x00}; // target_channel 0

struct RecvFrameE220900T22SJP_t data;

/** prototype declaration **/
void LoRaRecvTask(void *pvParameters);
void InfluxSendTask(void *pvParameters);

void setup() {

  SerialMon.begin(9600);
  delay(2000); // SerialMon init wait
  //SerialMon.println();
  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  //Setup OTA
  ArduinoOTA.setHostname("XiaoLoraGW01");
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println();
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  Serial.print("WiFi connected\r\nIP address: ");
  Serial.println(WiFi.localIP());
  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  
  // LoRa設定値の読み込み
  
  if (lora.LoadConfigSetting(CONFIG_FILENAME, config)) {
    //SerialMon.printf("Loading Configfile failed. The default value is set.\n");
    delay(1);
  } else {
    //SerialMon.printf("Loading Configfile succeeded.\n");
    delay(1);
  }

  // E220-900T22S(JP)へのLoRa初期設定
  if (lora.InitLoRaModule(config)) {
    //SerialMon.printf("init error\n");
    return;
  }

  // ノーマルモード(M0=0,M1=0)へ移行する
  lora.SwitchToNormalMode();

  // マルチタスク
  xTaskCreateUniversal(LoRaRecvTask, "LoRaRecvTask", 8192, NULL, 1, NULL, 0);
  xTaskCreateUniversal(InfluxSendTask, "InfluxSendTask", 8192, NULL, 1, NULL, 0);
}


void loop() {
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
  delay(1000);
  if (millis() > resettime){
    WiFi.disconnect(true);
    esp_restart();
  }
}
void InfluxSendTask(void *pvParameters) {
  while(1){
    if (toInflux == 1){ 
      delay(10);
      // Clear fields for reusing the point. Tags will remain the same as set above.
      sensor.clearFields();
      sensor.clearTags();
      // Store measured value into point
      // Report RSSI of currently connected network
      char strID[10];
      char strlat[10],strlon[10];
      sprintf(strID, "%d", sensorID);
      sprintf(strlat, "%3.7f", lat);
      sprintf(strlon, "%3.7f", lon);
      // sensor.addTag("device", "LoraXiao");
      sensor.addTag("ID",strID);
      sensor.addField("rssi", int(data.rssi));
      sensor.addField("bootcount",int(bootcount));
      sensor.addField("temperature",temp.floatformat);
      sensor.addField("lat",lat,7);
      sensor.addField("lon",lon,7);
      sensor.addField("water",int(water));
      // Print what are we exactly writing
      Serial.print("Writing: ");
      Serial.println(sensor.toLineProtocol());
      delay(10);
      // Check WiFi connection and reconnect if needed
      if (wifiMulti.run() != WL_CONNECTED) {
        Serial.println("Wifi connection lost");
      }
      delay(10);
      // Write point
      if (!client.writePoint(sensor)) {
        Serial.print("InfluxDB write failed: ");
        Serial.println(client.getLastErrorMessage());
      }else{
      toInflux = false;
      // delay(100);
      Serial.println("Influx send OK");
      Serial.printf("total time %d\n",millis()-t);
      //SerialMon.flush();
      //delay(100);
      }
    }
    delay(10);
  }
}

void LoRaRecvTask(void *pvParameters) {
  while (1) {
    if (lora.RecieveFrame(&data) == 0) {
      t = millis();
      sensorID = data.recv_data[3] | (data.recv_data[4]<<8) ;
      water = data.recv_data[5] | (data.recv_data[6]<<8) ;
      bootcount = data.recv_data[7] | (data.recv_data[8]<<8) ;
      temp.byteformat[0] = data.recv_data[9];
      temp.byteformat[1] = data.recv_data[10];
      temp.byteformat[2] = data.recv_data[11];
      temp.byteformat[3] = data.recv_data[12];
      int c =0;
      // 緯度経度
      lat = 34.000000;
      lon = 136.000000;
      for (int i = 0; i <sizeof(pointDictionaryArr)/12; i++)
      {
        c=i;
        if (pointDictionaryArr[i].field_number == sensorID){
          lat = pointDictionaryArr[i].point_lat;
          lon = pointDictionaryArr[i].point_lon;
          break;
        }
      }
      delay(1);
      toInflux = true;
    }
    delay(10);
  }
}
