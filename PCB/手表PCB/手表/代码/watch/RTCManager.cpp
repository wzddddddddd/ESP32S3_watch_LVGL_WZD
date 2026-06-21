// RTCManager.cpp
#include "RTCManager.h"
#include "time.h"

RTCManager rtcManager;

RTCManager::RTCManager() : initialized(false), rtcAvailable(false) {}

bool RTCManager::begin() {
    if (initialized) return rtcAvailable;
    
    Serial.println("初始化外部RTC...");
    
    // 初始化I2C
    //Wire.begin(SDA_PIN, SCL_PIN);
    //Wire.setClock(100000);
    
    // 检查设备是否存在
    Wire.beginTransmission(SD3078_I2C_ADDR);
    if (Wire.endTransmission() == 0) {
        Serial.println("检测到外部RTC(SD3078)");
        rtcAvailable = true;
        
        // 禁用电池充电（硬件已接地）
      if (writeEnable() == 0) {
        writeByte(SD3078_REG_CHARGE, 0x00); // 完全禁用充电功能
        writeDisable();
        Serial.println("RTC电池充电已禁用");
      }
    } else {
        Serial.println("未检测到外部RTC");
        rtcAvailable = false;
    }
    
    initialized = true;
    return rtcAvailable;
}

bool RTCManager::isAvailable() {
    return rtcAvailable;
}

bool RTCManager::readTime(DateTime* dt) {
    if (!rtcAvailable) return false;
    
    uint8_t data[7];
    if (readBytes(SD3078_REG_SEC, data, 7) != 0) {
        Serial.println("读取RTC时间失败");
        return false;
    }
    
    dt->second = bcdToHex(data[0] & 0x7F);
    dt->minute = bcdToHex(data[1] & 0x7F);
    
    // 处理小时寄存器
    if (data[2] & 0x80) { // 24小时制
        dt->hour = bcdToHex(data[2] & 0x3F);
    } else { // 12小时制
        dt->hour = bcdToHex(data[2] & 0x1F);
    }
    
    dt->week = bcdToHex(data[3] & 0x07);
    dt->day = bcdToHex(data[4] & 0x3F);
    dt->month = bcdToHex(data[5] & 0x1F);
    dt->year = bcdToHex(data[6]);
    
    return true;
}

bool RTCManager::setTime(const DateTime* dt) {
    if (!rtcAvailable) return false;
    
    uint8_t data[7];
    data[0] = hexToBcd(dt->second);
    data[1] = hexToBcd(dt->minute);
    data[2] = hexToBcd(dt->hour) | 0x80; // 24小时制
    data[3] = hexToBcd(dt->week);
    data[4] = hexToBcd(dt->day);
    data[5] = hexToBcd(dt->month);
    data[6] = hexToBcd(dt->year);
    
    // 解除写入保护
    if (writeEnable() != 0) {
        Serial.println("RTC写入使能失败");
        return false;
    }
    
    // 写入时间数据
    Wire.beginTransmission(SD3078_I2C_ADDR);
    Wire.write(SD3078_REG_SEC);
    for (int i = 0; i < 7; i++) {
        Wire.write(data[i]);
    }
    uint8_t result = Wire.endTransmission();
    
    // 恢复写入保护
    writeDisable();
    
    if (result == 0) {
        Serial.println("RTC时间设置成功");
        return true;
    } else {
        Serial.println("RTC时间设置失败");
        return false;
    }
}

bool RTCManager::syncToSystem() {
    if (!rtcAvailable) return false;
    
    DateTime rtcTime;
    if (!readTime(&rtcTime)) return false;
    
    // 转换为系统时间
    struct tm sysTime;
    sysTime.tm_sec = rtcTime.second;
    sysTime.tm_min = rtcTime.minute;
    sysTime.tm_hour = rtcTime.hour;
    sysTime.tm_mday = rtcTime.day;
    sysTime.tm_mon = rtcTime.month - 1;
    sysTime.tm_year = rtcTime.year + 100; // 2000年开始
    
    time_t epochTime = mktime(&sysTime);
    struct timeval tv = {epochTime, 0};
    
    if (settimeofday(&tv, NULL) == 0) {
        Serial.println("系统时间已从RTC同步");
        return true;
    }
    
    return false;
}

bool RTCManager::syncFromSystem() {
    if (!rtcAvailable) return false;
    
    time_t now;
    time(&now);
    struct tm* sysTime = localtime(&now);
    
    DateTime rtcTime;
    rtcTime.second = sysTime->tm_sec;
    rtcTime.minute = sysTime->tm_min;
    rtcTime.hour = sysTime->tm_hour;
    rtcTime.day = sysTime->tm_mday;
    rtcTime.month = sysTime->tm_mon + 1;
    rtcTime.year = sysTime->tm_year - 100; 
    rtcTime.week = sysTime->tm_wday == 0 ? 7 : sysTime->tm_wday;
    
    return setTime(&rtcTime);
}

// 私有方法实现
uint8_t RTCManager::writeByte(uint8_t reg, uint8_t data) {
    Wire.beginTransmission(SD3078_I2C_ADDR);
    Wire.write(reg);
    Wire.write(data);
    return Wire.endTransmission();
}

uint8_t RTCManager::readByte(uint8_t reg) {
    Wire.beginTransmission(SD3078_I2C_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(SD3078_I2C_ADDR, 1);
    return Wire.read();
}

uint8_t RTCManager::readBytes(uint8_t reg, uint8_t *data, uint8_t len) {
    Wire.beginTransmission(SD3078_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return 1;
    }
    
    Wire.requestFrom(SD3078_I2C_ADDR, len);
    for (uint8_t i = 0; i < len; i++) {
        if (Wire.available()) {
            data[i] = Wire.read();
        } else {
            return 1;
        }
    }
    return 0;
}

uint8_t RTCManager::bcdToHex(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

uint8_t RTCManager::hexToBcd(uint8_t hex) {
    return ((hex / 10) << 4) | (hex % 10);
}

uint8_t RTCManager::writeEnable() {
    // 设置WRTC1=1
    uint8_t ctr2 = readByte(SD3078_REG_CTR2);
    ctr2 |= WRTC1_BIT;
    if (writeByte(SD3078_REG_CTR2, ctr2) != 0) {
        return 1;
    }
    
    // 设置WRTC2=1和WRTC3=1
    uint8_t ctr1 = readByte(SD3078_REG_CTR1);
    ctr1 |= (WRTC2_BIT | WRTC3_BIT);
    if (writeByte(SD3078_REG_CTR1, ctr1) != 0) {
        return 1;
    }
    
    return 0;
}

uint8_t RTCManager::writeDisable() {
    // 清除WRTC2和WRTC3
    uint8_t ctr1 = readByte(SD3078_REG_CTR1);
    ctr1 &= ~(WRTC2_BIT | WRTC3_BIT);
    if (writeByte(SD3078_REG_CTR1, ctr1) != 0) {
        return 1;
    }
    
    // 清除WRTC1
    uint8_t ctr2 = readByte(SD3078_REG_CTR2);
    ctr2 &= ~WRTC1_BIT;
    if (writeByte(SD3078_REG_CTR2, ctr2) != 0) {
        return 1;
    }
    
    return 0;
}