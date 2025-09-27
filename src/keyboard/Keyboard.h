#pragma once
#include <Arduino.h>
#include <Wire.h>

class Keyboard {
public:
  void begin(uint8_t addr) {
    _addr = addr;
    _head = _tail = 0;
  }

  void poll() {
    // Attempt to read up to a few bytes per poll from I2C keyboard
    // This is a generic stub. Many T-Keyboard firmwares expose a FIFO at 0x00.
    for (int i = 0; i < 8; ++i) {
      Wire.beginTransmission(_addr);
      // Request a single key byte; adjust register protocol if needed
      // For some firmwares, a 0 value means no key
      Wire.write(0x00);
      if (Wire.endTransmission(false) != 0) {
        // device no-ack; stop polling further
        break;
      }
      if (Wire.requestFrom((int)_addr, 1) == 1) {
        uint8_t c = Wire.read();
        if (c != 0x00) {
          push(c);
        } else {
          // no more keys this poll
          break;
        }
      } else {
        break;
      }
    }
  }

  bool available() const { return _head != _tail; }

  uint8_t read() {
    if (_head == _tail) return 0;
    uint8_t c = _buf[_tail];
    _tail = (_tail + 1) & (BUF_SIZE-1);
    return c;
  }

private:
  static constexpr int BUF_SIZE = 128;
  uint8_t _buf[BUF_SIZE];
  volatile uint16_t _head=0, _tail=0;
  uint8_t _addr=0x55;

  void push(uint8_t c) {
    uint16_t next = (_head + 1) & (BUF_SIZE-1);
    if (next != _tail) {
      _buf[_head] = c;
      _head = next;
    }
  }
};
