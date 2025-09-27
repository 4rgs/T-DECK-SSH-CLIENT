#pragma once

#include <Arduino.h>
#include <Wire.h>

// GT911 I2C addresses
#define GT911_SLAVE_ADDRESS_L  0x5D  // 0xBA >> 1
#define GT911_SLAVE_ADDRESS_H  0x14  // 0x28 >> 1

// GT911 Registers
#define GT911_POINT_INFO       0x814E
#define GT911_POINT_1          0x814F
#define GT911_POINT_2          0x8157
#define GT911_POINT_3          0x815F
#define GT911_POINT_4          0x8167
#define GT911_POINT_5          0x816F

#define GT911_CLEARBUF         0x814E
#define GT911_CONFIG_REG       0x8047
#define GT911_COMMAND_REG      0x8040
#define GT911_PRODUCT_ID       0x8140

class TouchDrvGT911 {
public:
    TouchDrvGT911();
    ~TouchDrvGT911();
    
    bool begin(TwoWire &wire, uint8_t addr = GT911_SLAVE_ADDRESS_L, int sda = -1, int scl = -1);
    void setPins(int rst, int irq);
    void setMaxCoordinates(int16_t x, int16_t y);
    void setSwapXY(bool swap);
    void setMirrorXY(bool mirrorX, bool mirrorY);
    
    uint8_t getPoint(int16_t *x_array, int16_t *y_array, uint8_t get_point = 1);
    bool isPressed();
    
private:
    TwoWire* _wire;
    uint8_t _addr;
    int _rst;
    int _irq;
    int16_t _maxX;
    int16_t _maxY;
    bool _swapXY;
    bool _mirrorX;
    bool _mirrorY;
    
    bool writeRegister(uint16_t reg, uint8_t *data, uint8_t len);
    bool readRegister(uint16_t reg, uint8_t *data, uint8_t len);
    uint32_t getChipID();
};