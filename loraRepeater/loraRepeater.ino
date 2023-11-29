#include "esp32_e220900t22s_jp_lib.h"
#include <Arduino.h>
#include <FS.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <esp_pm.h>
#define OWN_ADDRESS 0xffff
#define RCV_CH 0x09
#define SEND_CH 0x00
#define WOR_PIN GPIO_NUM_10
CLoRa lora;

struct LoRaConfigItem_t config = {
      OWN_ADDRESS, // own_address 0
      0b011, // baud_rate 9600 bps
      0b10000, // air_data_rate SF:9 BW:125
      0b11, // subpacket_size 200
      0b1, // rssi_ambient_noise_flag 有効
      0b0, // transmission_pause_flag 有効
      0b01, // transmitting_power 13 dBm
      RCV_CH, // own_channel 0f
      0b1, // rssi_byte_flag 有効
      0b0, // transmission_method_type トランスペアレント送信モード(default)
      0b0, // lbt_flag 有効
      0b011, // wor_cycle 2000 ms
      0x0000, // encryption_key 0
      0xFFFF, // target_address 0
      0x00}; // target_channel 0

struct RecvFrameE220900T22SJP_t data;

// 受信用構造体　送信用と一致させる
typedef struct struct_message {
  uint16_t address;
  uint16_t bootcount;
  bool water_high;
  bool water_low;
} struct_message;

struct_message recvData;
esp_now_peer_info_t slave;
/** prototype declaration **/
void LoRaRecvTask(void *pvParameters);
void LoRaSendTask(void *pvParameters);
void ReadDataFromConsole(char *msg, int max_msg_len);
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) ;
//WDT
#include "esp_system.h"


struct  __attribute__((packed, aligned(4))) msgStruct{ 
  uint16_t adress ;
  uint16_t water ;
  uint16_t bootcount;
  float temp = -127  ;
} sendData;

union ByteFloatUnion{
  uint8_t byteformat[4];
  float floatformat;
} temp;

const int wdtTimeout = 30*1000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;
bool sendflg=0;

// ESP-NOW受信コールバック関数
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  memcpy(&recvData, incomingData, sizeof(recvData));
  lora.SwitchToNormalMode();
  sendData.adress = recvData.address;
  config.own_address = sendData.adress;
  config.own_channel = SEND_CH;
  // config.target_channel = SEND_CH;
  sendData.water = (recvData.water_low * 49) + (recvData.water_high * 51) ;
  sendData.bootcount = recvData.bootcount;
  Serial.print("ESPNOW recv");
  Serial.print(len);
  Serial.println(" Bytes received");
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
          mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("mac addr: "); Serial.println(macStr);
  Serial.print("Sender adress: ");
  Serial.println(sendData.adress);
  Serial.print("bootcount: ");
  Serial.println(sendData.bootcount);
  Serial.print("water Status: ");
  Serial.println(sendData.water);
  delay(10);
  while(!digitalRead(LoRa_AUXPin)){}
  if (lora.InitLoRaModule(config)) {
    SerialMon.printf("init error\n");
  }
  while(!digitalRead(LoRa_AUXPin)){}
  lora.SwitchToNormalMode();
  SerialLoRa.flush();
  // lora.SwitchToWORSendingMode();
  delay(100);
  while(!digitalRead(LoRa_AUXPin)){}
  sendflg=1;

}
//LORA 9ch
void LoRaRecvTask(void *pvParameters) {
  while (1) {
    if ( lora.RecieveFrame(&data) == 0 ){
      delay(10);
      SerialMon.printf("Lora recv\n");
      sendData.adress = data.recv_data[3] | (data.recv_data[4]<<8) ;
      sendData.water = data.recv_data[5] | (data.recv_data[6]<<8) ;
      sendData.bootcount = data.recv_data[7] | (data.recv_data[8]<<8) ;
      // temp.byteformat[0] = data.recv_data[9];
      // temp.byteformat[1] = data.recv_data[10];
      // temp.byteformat[2] = data.recv_data[11];
      // temp.byteformat[3] = data.recv_data[12];
      // sendData.temp = temp.floatformat;
      // lora.SwitchToNormalMode();
      // SerialLoRa.flush();
      config.own_address = sendData.adress;
      config.own_channel = SEND_CH;
      // config.target_channel = SEND_CH;
      delay(10);
      while(!digitalRead(LoRa_AUXPin)){}
      if (lora.InitLoRaModule(config)) {
        SerialMon.printf("init error\n");
      }
      while(!digitalRead(LoRa_AUXPin)){}
      lora.SwitchToNormalMode();
      SerialLoRa.flush();
      // lora.SwitchToWORSendingMode();
      delay(100);
      while(!digitalRead(LoRa_AUXPin)){}
      sendflg=1;
      // SerialMon.flush();
      }
    
    delay(1000);
  }
}
void setup() {
  // esp_pm_config_esp32c3_t pm_config;
  // pm_config.max_freq_mhz = 80;  // 周波数を80 MHzに設定
  // pm_config.min_freq_mhz = 80;  // 周波数を80 MHzに設定
  // esp_pm_configure(&pm_config);
  SerialMon.begin(115200);
  while (SerialMon.available()<0){}
  delay(500); // SerialMon init wait
  SerialMon.printf("start\n");
  // pinMode(LoRa_AUXPin,INPUT_PULLUP  );
  // if (lora.LoadConfigSetting(CONFIG_FILENAME, config)) {
  //   SerialMon.printf("Loading Configfile failed. The default value is set.\n");
  // } else {
  //   SerialMon.printf("Loading Configfile succeeded.\n");
  // }

  if (lora.InitLoRaModule(config)) {
    SerialMon.printf("init error\n");
  }
  delay(100);
  while(!digitalRead(LoRa_AUXPin)){}
  lora.SwitchToNormalMode();
  while(!digitalRead(LoRa_AUXPin)){}
  SerialLoRa.flush();
   // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

//ESP-NOW受信コールバック関数登録
  esp_now_register_recv_cb(OnDataRecv);
  WiFi.disconnect();
  SerialMon.printf("esp-now waiting\n");
// マルチタスク
  xTaskCreateUniversal(LoRaRecvTask, "LoRaRecvTask", 8192, NULL, 1, NULL,
                       0);
}

void loop() {
  if (sendflg){
    SerialMon.printf("I recv data\n");
    if (lora.SendFrame(config, (uint8_t *)&sendData, sizeof(sendData)) == 0) {
      delay(500);
      SerialMon.printf("send succeeded.\n");
      SerialMon.printf("\n");
      sendflg=0;
    } else {
      SerialMon.printf("send failed.\n");
      SerialMon.printf("\n");
    }
    ESP.restart();
    
  }
  delay(10);
}
