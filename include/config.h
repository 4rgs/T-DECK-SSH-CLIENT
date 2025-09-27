#pragma once

// ==== User Configuration (edit and rebuild) ====

// WiFi
#define WIFI_SSID            "spidsNET2.4G"
#define WIFI_PASS            "1314162234a"

// SSH bridge TCP server (your Python script host)
#define BRIDGE_HOST          "192.168.1.81"   // set your server IP/hostname
#define BRIDGE_PORT          2324

// Display
#define LCD_WIDTH            320
#define LCD_HEIGHT           240
#define LCD_ROTATION         1     // 0..3

// LovyanGFX config mode
// 1 = use LGFX auto-detect (simplest). 0 = use manual pin mapping below
#define USE_LGFX_AUTODETECT  0

// Manual SPI pin map (T-Deck official pins)
#define LCD_PIN_SCLK         40    // BOARD_SPI_SCK
#define LCD_PIN_MOSI         41    // BOARD_SPI_MOSI
#define LCD_PIN_MISO         38    // BOARD_SPI_MISO (not used for write-only)
#define LCD_PIN_CS           12    // BOARD_TFT_CS
#define LCD_PIN_DC           11    // BOARD_TFT_DC
#define LCD_PIN_RST          -1    // No reset pin used
#define LCD_PIN_BL           42    // BOARD_TFT_BACKLIGHT

// Keyboard (LILYGO T-KEYBOARD over I2C) - Official pins
#define KEYBOARD_I2C_ADDR    0x55   // T-Deck keyboard address
#define KEYBOARD_I2C_SDA     18     // BOARD_I2C_SDA
#define KEYBOARD_I2C_SCL     8      // BOARD_I2C_SCL
#define KEYBOARD_POLL_MS     10

// Terminal
#define TERM_FONT_WIDTH      6      // built-in small font width (approx)
#define TERM_FONT_HEIGHT     8      // built-in small font height (approx)
#define TERM_MARGIN_X        2
#define TERM_MARGIN_Y        2
#define TERM_STATUS_BAR      1      // draw a top status bar with connection info

// Reconnect/backoff
#define WIFI_RETRY_DELAY_MS  2000
#define TCP_RETRY_DELAY_MS   2000

// Debug
#define ENABLE_SERIAL_DEBUG  1
