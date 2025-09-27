#include "TouchDrvGT911.hpp"

TouchDrvGT911::TouchDrvGT911() : _wire(nullptr), _addr(GT911_SLAVE_ADDRESS_L), 
                                 _rst(-1), _irq(-1), _maxX(320), _maxY(240), 
                                 _swapXY(false), _mirrorX(false), _mirrorY(false) {
}

TouchDrvGT911::~TouchDrvGT911() {
}

bool TouchDrvGT911::begin(TwoWire &wire, uint8_t addr, int sda, int scl) {
    _wire = &wire;
    _addr = addr;
    
    if (sda != -1 && scl != -1) {
        _wire->begin(sda, scl);
    }
    
    // Esperar un poco para estabilizar la comunicación I2C
    delay(100);
    
    // Intentar leer el registro de estado primero
    uint8_t status = 0;
    bool hasStatus = readRegister(GT911_POINT_INFO, &status, 1);
    
    // Try to read chip ID to verify communication
    uint32_t id = getChipID();
    
    Serial.printf("GT911 Debug - Addr:0x%02X, Status:%s (0x%02X), ID:0x%08X\n", 
                  _addr, hasStatus?"OK":"FAIL", status, id);
    
    return (id != 0 || hasStatus);
}

void TouchDrvGT911::setPins(int rst, int irq) {
    _rst = rst;
    _irq = irq;
    
    if (_irq != -1) {
        pinMode(_irq, INPUT);
    }
}

void TouchDrvGT911::setMaxCoordinates(int16_t x, int16_t y) {
    _maxX = x;
    _maxY = y;
}

void TouchDrvGT911::setSwapXY(bool swap) {
    _swapXY = swap;
}

void TouchDrvGT911::setMirrorXY(bool mirrorX, bool mirrorY) {
    _mirrorX = mirrorX;
    _mirrorY = mirrorY;
}

uint8_t TouchDrvGT911::getPoint(int16_t *x_array, int16_t *y_array, uint8_t get_point) {
    if (!_wire || !x_array || !y_array) return 0;
    
    uint8_t pointInfo = 0;
    if (!readRegister(GT911_POINT_INFO, &pointInfo, 1)) {
        return 0;
    }
    
    uint8_t touchCount = pointInfo & 0x0F;
    

    
    if (touchCount == 0 || !(pointInfo & 0x80)) {
        // Clear buffer solo si hay datos válidos
        if (pointInfo != 0) {
            uint8_t clear = 0;
            writeRegister(GT911_CLEARBUF, &clear, 1);
        }
        return 0;
    }
    
    // Read touch points
    uint8_t pointData[8];
    uint16_t pointReg = GT911_POINT_1;
    
    for (uint8_t i = 0; i < touchCount && i < get_point; i++) {
        if (readRegister(pointReg, pointData, 8)) {
            // Formato GT911: X(low) X(high) Y(low) Y(high) Size Pressure Reserved Reserved
            uint16_t rawX = (pointData[1] << 8) | pointData[0];
            uint16_t rawY = (pointData[3] << 8) | pointData[2];
            
            // Escalar desde resolución nativa del GT911 a la pantalla
            int16_t x = (rawX * _maxX) / 65536;
            int16_t y = (rawY * _maxY) / 65536;
            
            // Aplicar transformaciones configuradas
            if (_swapXY) {
                int16_t temp = x;
                x = y;
                y = temp;
            }
            
            if (_mirrorX) {
                x = _maxX - x;
            }
            
            if (_mirrorY) {
                y = _maxY - y;
            }
            
            // Clamp to valid range
            x = constrain(x, 0, _maxX);
            y = constrain(y, 0, _maxY);
            x_array[i] = x;
            y_array[i] = y;
        }
        
        pointReg += 8; // Next point register
    }
    
    // Clear buffer
    uint8_t clear = 0;
    writeRegister(GT911_CLEARBUF, &clear, 1);
    
    return touchCount;
}

bool TouchDrvGT911::isPressed() {
    if (!_wire) return false;
    
    uint8_t pointInfo = 0;
    if (!readRegister(GT911_POINT_INFO, &pointInfo, 1)) {
        return false;
    }
    
    return (pointInfo & 0x80) && (pointInfo & 0x0F);
}

bool TouchDrvGT911::writeRegister(uint16_t reg, uint8_t *data, uint8_t len) {
    if (!_wire) return false;
    
    _wire->beginTransmission(_addr);
    _wire->write((reg >> 8) & 0xFF);
    _wire->write(reg & 0xFF);
    
    for (uint8_t i = 0; i < len; i++) {
        _wire->write(data[i]);
    }
    
    return _wire->endTransmission() == 0;
}

bool TouchDrvGT911::readRegister(uint16_t reg, uint8_t *data, uint8_t len) {
    if (!_wire) return false;
    
    _wire->beginTransmission(_addr);
    _wire->write((reg >> 8) & 0xFF);
    _wire->write(reg & 0xFF);
    
    if (_wire->endTransmission() != 0) {
        return false;
    }
    
    _wire->requestFrom(_addr, len);
    
    for (uint8_t i = 0; i < len; i++) {
        if (_wire->available()) {
            data[i] = _wire->read();
        } else {
            return false;
        }
    }
    
    return true;
}

uint32_t TouchDrvGT911::getChipID() {
    uint8_t idData[4];
    if (!readRegister(GT911_PRODUCT_ID, idData, 4)) {
        return 0;
    }
    
    return (idData[3] << 24) | (idData[2] << 16) | (idData[1] << 8) | idData[0];
}