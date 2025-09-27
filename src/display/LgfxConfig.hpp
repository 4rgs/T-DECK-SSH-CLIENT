#pragma once

#include <LovyanGFX.hpp>
#include "config.h"

// Define custom LGFX class for this project
class CustomLGFX : public lgfx::LGFX_Device {
public:
  CustomLGFX(void) {
#if USE_LGFX_AUTODETECT
    // Use LovyanGFX auto-detect. Works for many ESP32-S3 boards.
    auto cfg = _bus_instance.config();
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);

    { // Panel config
      auto pcfg = _panel_instance.config();
      pcfg.memory_width  = LCD_WIDTH;
      pcfg.memory_height = LCD_HEIGHT;
      pcfg.panel_width   = LCD_WIDTH;
      pcfg.panel_height  = LCD_HEIGHT;
      _panel_instance.config(pcfg);
    }

    setPanel(&_panel_instance);
#else
    // Manual SPI bus and panel configuration
    { // SPI Bus
      auto cfg = _bus_instance.config();
      cfg.spi_host   = SPI2_HOST;          // VSPI or FSPI for S3
      cfg.spi_mode   = 0;
      cfg.freq_write = 40000000;           // 40MHz
      cfg.pin_sclk   = LCD_PIN_SCLK;
      cfg.pin_mosi   = LCD_PIN_MOSI;
      cfg.pin_miso   = LCD_PIN_MISO;
      cfg.pin_dc     = LCD_PIN_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    { // Panel
      auto cfg = _panel_instance.config();
      cfg.pin_cs       = LCD_PIN_CS;
      cfg.pin_rst      = LCD_PIN_RST;
      cfg.pin_busy     = -1;
      cfg.memory_width  = LCD_WIDTH;
      cfg.memory_height = LCD_HEIGHT;
      cfg.panel_width   = LCD_WIDTH;
      cfg.panel_height  = LCD_HEIGHT;
      cfg.offset_x      = 0;
      cfg.offset_y      = 0;
      cfg.readable      = false;
      cfg.invert        = false;
      cfg.rgb_order     = false;
      cfg.dlen_16bit    = false;
      cfg.bus_shared    = true;
      _panel_instance.config(cfg);
    }

    { // Backlight (optional GPIO)
      auto cfg = _light_instance.config();
      cfg.pin_bl = LCD_PIN_BL;
      cfg.invert = false;
      cfg.freq   = 44100;
      cfg.pwm_channel = 7;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
#endif
  }

private:
  lgfx::Bus_SPI _bus_instance;
  lgfx::Panel_ST7789 _panel_instance; // Many 320x240 LCDs are ST7789; adjust if needed
  lgfx::Light_PWM _light_instance;
};
