#include <Arduino.h>
#include <M5unified.h>
#include <FS.h>
#include "SPIFFS.h"
#include "Unit_MiniEncoderC.h"
#define I2C_DEV_ADDR 0x55

UNIT_MINIENCODERC sensor;

uint16_t delay_time = 0;
// int32_t last_value  = 0;

uint8_t i2c_address = 0;

long oldPosition = 0;
unsigned long lastEncoderUpdateTime = 0;
int16_t oldEncoderValue = 0;
int16_t newEncoderValue = 0;
int16_t newPosition = 0;
int16_t encoderDelta = 0;
int16_t encoderChangeThreshold = 5; // 変化量のしきい値


void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setTextColor(GREEN);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextFont(&fonts::Orbitron_Light_32);
    M5.Display.setTextSize(2);
    M5.Display.setRotation(1);
    // Wire.begin(32,33,100000);
    sensor.begin(&Wire, MINIENCODERC_ADDR, 0, 26, 100000UL);

    delay_time  = 20;
    i2c_address = sensor.getI2CAddress();
    M5.Display.drawString(String("ON"),
                    M5.Display.width() / 2,
                    M5.Display.height() / 2);
    // sensor.setEncoderValue(97900);
}

// void loop() {
//     M5.update();
//     if (M5.BtnA.wasPressed()) {
//         get_inc_value();
//     } else if (M5.BtnB.wasPressed()) {
//         sensor.resetCounter();
//     }
//     page_get_encoder();
// }
void loop() {
    M5.update();
    // エンコーダーの値を読み取り
    newEncoderValue = int16_t(sensor.getEncoderValue());

    // エンコーダーの変化を時間で追跡
    unsigned long currentTime = millis();
    unsigned long deltaTime = currentTime - lastEncoderUpdateTime;

    if (deltaTime >= 50) { // 50ミリ秒ごとに計算
        // エンコーダーの変化量を計算
        encoderDelta =     newEncoderValue - oldEncoderValue;

        // 変化量がしきい値を超えていれば値を変更
        if (abs(encoderDelta) > 0) {
            newPosition = oldPosition + (encoderDelta*abs(encoderDelta));
            // M5.Speaker.tone(8000, 20);
            M5.Display.clear();
            oldPosition = newPosition;
            // Serial.println(newPosition);
            M5.Display.drawString(String(newPosition),
                                  M5.Display.width() / 2,
                                  M5.Display.height() / 2);
            oldEncoderValue = newEncoderValue;
            oldPosition = newPosition;
            lastEncoderUpdateTime = currentTime;
        }
        lastEncoderUpdateTime = currentTime;
    }

    if (M5.BtnA.wasPressed()) {
        Serial.println(newPosition);
        Wire.beginTransmission(I2C_DEV_ADDR);
        delay(100);
        Wire.write(newPosition>>8);
        Wire.write(newPosition&0xff);
        uint8_t error = Wire.endTransmission(true);
        Serial.printf("endTransmission: %u\n", error);
    }
    if (!sensor.getButtonStatus()){
        sensor.resetCounter(); 
        newPosition =0;
        M5.Display.clear();
        M5.Display.drawString(String(newPosition),
                            M5.Display.width() / 2,
                            M5.Display.height() / 2);
        oldEncoderValue = 0;
        oldPosition =0;
        lastEncoderUpdateTime = currentTime;
        delay(100);
    }
    if (M5.BtnB.wasPressed()) {
        sensor.resetCounter();
        M5.Power.timerSleep(1);
    }
    if (millis() > 60*1000){
        M5.Power.powerOff();
    }
}
