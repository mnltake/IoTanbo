#ifndef __UNIT_MINIENCODERC_H
#define __UNIT_MINIENCODERC_H

#include "Arduino.h"
#include "Wire.h"

#define MINIENCODERC_ADDR    0x42
#define ENCODER_REG          0x00
#define INCREMENT_REG        0x10
#define BUTTON_REG           0x20
#define RGB_LED_REG          0x30
#define RESET_COUNTER_REG    0x40
#define FIRMWARE_VERSION_REG 0xFE
#define I2C_ADDRESS_REG      0xFF

class UNIT_MINIENCODERC {
   private:
    uint8_t _addr;
    TwoWire* _wire;
    uint8_t _scl;
    uint8_t _sda;
    uint8_t _speed;
    void writeBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);
    void readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);

   public:
    bool begin(TwoWire* wire = &Wire, uint8_t addr = MINIENCODERC_ADDR,
               uint8_t sda = 21, uint8_t scl = 22, uint32_t speed = 100000L);
    int32_t getEncoderValue();
    void setEncoderValue(int32_t value);
    uint32_t getIncrementValue();
    bool getButtonStatus();
    void setLEDColor(uint8_t index, uint32_t color);
    uint8_t setI2CAddress(uint8_t addr);
    uint8_t getI2CAddress(void);
    uint8_t getFirmwareVersion(void);
    void resetCounter(void);
};

#endif
