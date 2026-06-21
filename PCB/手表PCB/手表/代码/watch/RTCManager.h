// RTCManager.h
#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Wire.h>
#include <Arduino.h>

struct DateTime {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t week;
  uint8_t day;
  uint8_t month;
  uint8_t year;
};

class RTCManager {
public:
    RTCManager();
    bool begin();
    bool isAvailable();
    bool readTime(DateTime* dt);
    bool setTime(const DateTime* dt);
    bool syncToSystem();
    bool syncFromSystem();
    
private:
    bool initialized;
    bool rtcAvailable;
    
    // SD3078 寄存器地址
    static const uint8_t SD3078_I2C_ADDR = 0x32;
    static const uint8_t SD3078_REG_SEC = 0x00;
    static const uint8_t SD3078_REG_MIN = 0x01;
    static const uint8_t SD3078_REG_HOUR = 0x02;
    static const uint8_t SD3078_REG_WDAY = 0x03;
    static const uint8_t SD3078_REG_MDAY = 0x04;
    static const uint8_t SD3078_REG_MON = 0x05;
    static const uint8_t SD3078_REG_YEAR = 0x06;
    static const uint8_t SD3078_REG_CTR1 = 0x0F;
    static const uint8_t SD3078_REG_CTR2 = 0x10;
    static const uint8_t SD3078_REG_CHARGE = 0x18;
    
    // 控制位
    static const uint8_t WRTC1_BIT = 0x80;
    static const uint8_t WRTC2_BIT = 0x04;
    static const uint8_t WRTC3_BIT = 0x80;
    
    // 引脚定义
    static const uint8_t SCL_PIN = 18;
    static const uint8_t SDA_PIN = 17;
    
    uint8_t writeByte(uint8_t reg, uint8_t data);
    uint8_t readByte(uint8_t reg);
    uint8_t readBytes(uint8_t reg, uint8_t *data, uint8_t len);
    uint8_t bcdToHex(uint8_t bcd);
    uint8_t hexToBcd(uint8_t hex);
    uint8_t writeEnable();
    uint8_t writeDisable();
};

extern RTCManager rtcManager;

#endif