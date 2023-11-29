#include "Unit_MiniEncoderC.h"

void UNIT_MINIENCODERC::writeBytes(uint8_t addr, uint8_t reg, uint8_t *buffer,
                                   uint8_t length) {
    _wire->beginTransmission(addr);
    _wire->write(reg);
    for (int i = 0; i < length; i++) {
        _wire->write(*(buffer + i));
    }
    _wire->endTransmission();
}

void UNIT_MINIENCODERC::readBytes(uint8_t addr, uint8_t reg, uint8_t *buffer,
                                  uint8_t length) {
    uint8_t index = 0;
    _wire->beginTransmission(addr);
    _wire->write(reg);
    _wire->endTransmission(false);
    _wire->requestFrom(addr, length);
    for (int i = 0; i < length; i++) {
        buffer[index++] = _wire->read();
    }
}

bool UNIT_MINIENCODERC::begin(TwoWire *wire, uint8_t addr, uint8_t sda,
                              uint8_t scl, uint32_t speed) {
    _wire  = wire;
    _addr  = addr;
    _sda   = sda;
    _scl   = scl;
    _speed = speed;
    _wire->begin(_sda, _scl, _speed);
    delay(10);
    _wire->beginTransmission(_addr);
    uint8_t error = _wire->endTransmission();
    if (error == 0) {
        return true;
    } else {
        return false;
    }
}

void UNIT_MINIENCODERC::setEncoderValue(int32_t value) {
    uint8_t data[4];

    data[0] = value & 0xff;
    data[1] = (value >> 8) & 0xff;
    data[2] = (value >> 16) & 0xff;
    data[3] = (value >> 24) & 0xff;
    writeBytes(_addr, ENCODER_REG, data, 4);
}

int32_t UNIT_MINIENCODERC::getEncoderValue() {
    uint8_t data[4];
    readBytes(_addr, ENCODER_REG, data, 4);
    int32_t value =
        data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    return value;
}

uint32_t UNIT_MINIENCODERC::getIncrementValue() {
    uint8_t data[4];
    readBytes(_addr, INCREMENT_REG, data, 4);
    uint32_t value =
        data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    return value;
}

bool UNIT_MINIENCODERC::getButtonStatus() {
    uint8_t data;
    readBytes(_addr, BUTTON_REG, &data, 1);
    return data;
}

void UNIT_MINIENCODERC::setLEDColor(uint8_t index, uint32_t color) {
    uint8_t data[4];
    data[2] = color & 0xff;
    data[1] = (color >> 8) & 0xff;
    data[0] = (color >> 16) & 0xff;
    writeBytes(_addr, RGB_LED_REG, data, 3);
}

void UNIT_MINIENCODERC::resetCounter(void) {
    uint8_t data[4];
    data[0] = 1;
    writeBytes(_addr, RESET_COUNTER_REG, data, 1);
}

uint8_t UNIT_MINIENCODERC::setI2CAddress(uint8_t addr) {
    _wire->beginTransmission(_addr);
    _wire->write(I2C_ADDRESS_REG);
    _wire->write(addr);
    _wire->endTransmission();
    _addr = addr;
    return _addr;
}

uint8_t UNIT_MINIENCODERC::getI2CAddress(void) {
    _wire->beginTransmission(_addr);
    _wire->write(I2C_ADDRESS_REG);
    _wire->endTransmission();

    uint8_t RegValue;

    _wire->requestFrom(_addr, 1);
    RegValue = Wire.read();
    return RegValue;
}

uint8_t UNIT_MINIENCODERC::getFirmwareVersion(void) {
    _wire->beginTransmission(_addr);
    _wire->write(FIRMWARE_VERSION_REG);
    _wire->endTransmission();

    uint8_t RegValue;

    _wire->requestFrom(_addr, 1);
    RegValue = Wire.read();
    return RegValue;
}
