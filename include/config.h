#ifndef CONFIG_H
#define CONFIG_H

//==============================================================================
// T-DECK SSH TERMINAL CONFIGURATION
//==============================================================================

// Hardware pins según documentación oficial T-Deck
#define BOARD_POWERON       10
#define BOARD_SPI_MOSI      41
#define BOARD_SPI_MISO      38
#define BOARD_SPI_SCK       40
#define BOARD_TFT_CS        12
#define BOARD_TFT_DC        11
#define BOARD_TFT_BACKLIGHT 42
#define BOARD_I2C_SDA       18
#define BOARD_I2C_SCL       8

// Dirección I2C del teclado T-Deck
#define KEYBOARD_I2C_ADDR   0x55

// Pines del touch y trackball
#define BOARD_TOUCH_INT     16
#define BOARD_TRACKBALL_UP    3
#define BOARD_TRACKBALL_DOWN  15
#define BOARD_TRACKBALL_LEFT  1
#define BOARD_TRACKBALL_RIGHT 2
#define BOARD_TRACKBALL_CLICK 0

// Pin de lectura de batería
#define BOARD_BAT_ADC       4

//==============================================================================
// DISPLAY CONFIGURATION
//==============================================================================

#define SCREEN_WIDTH        240
#define SCREEN_HEIGHT       320
#define FONT_SIZE           1
#define LINE_HEIGHT         16
#define CHAR_WIDTH          6

//==============================================================================
// TERMINAL CONFIGURATION
//==============================================================================

#define MAX_OUTPUT_LINES    100
#define MAX_INPUT_LENGTH    256
#define MAX_HISTORY_SIZE    20
#define CURSOR_BLINK_RATE   500

//==============================================================================
// NETWORK CONFIGURATION
//==============================================================================

#define SCAN_START_IP       1
#define SCAN_END_IP         50
#define SSH_TIMEOUT         5000
#define MAX_SAVED_NETWORKS  5
#define MAX_KNOWN_HOSTS     20

//==============================================================================
// UPDATE INTERVALS (milliseconds)
//==============================================================================

#define UPDATE_INTERVAL         50   // 20 FPS
#define KEY_CHECK_INTERVAL      10   // 100 Hz
#define TOUCH_CHECK_INTERVAL    30   // 33 Hz
#define BATTERY_UPDATE_INTERVAL 30000 // 30 seconds

//==============================================================================
// COLORS (RGB565 format)
//==============================================================================

#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_ORANGE        0xFD20
#define COLOR_GRAY          0x8410
#define COLOR_DARK_GRAY     0x4208

// Theme colors
#define THEME_BG            COLOR_BLACK
#define THEME_TEXT          COLOR_WHITE
#define THEME_HIGHLIGHT     COLOR_CYAN
#define THEME_PROMPT        COLOR_GREEN
#define THEME_ERROR         COLOR_RED
#define THEME_SUCCESS       COLOR_GREEN
#define THEME_WARNING       COLOR_YELLOW

//==============================================================================
// SYSTEM CONFIGURATION  
//==============================================================================

#define BATTERY_LOW_THRESHOLD   15    // Percentage
#define BACKLIGHT_DEFAULT       200   // 0-255
#define BACKLIGHT_TIMEOUT       60000 // 1 minute

//==============================================================================
// VERSION INFORMATION
//==============================================================================

#define FIRMWARE_VERSION    "2.0.0"
#define FIRMWARE_DATE       "2025-09-27"
#define FIRMWARE_AUTHOR     "Alvaro Gonzalez"

#endif

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
