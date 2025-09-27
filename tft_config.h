// T-Deck specific configuration for TFT_eSPI
// This file should be placed in the lib directory or TFT_eSPI User_Setup.h

// Driver for ST7789 320x240 display
#define ST7789_DRIVER

// T-Deck display size
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// T-Deck specific pins
#define TFT_MOSI 41  // BOARD_SPI_MOSI  
#define TFT_SCLK 40  // BOARD_SPI_SCK
#define TFT_CS   12  // BOARD_TFT_CS
#define TFT_DC   11  // BOARD_TFT_DC
#define TFT_RST  -1  // Not used
#define TFT_BL   42  // BOARD_TFT_BACKLIGHT

// SPI frequency
#define SPI_FREQUENCY  27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// Color order (may need adjustment)
#define TFT_RGB_ORDER TFT_RGB

// Fonts to load
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.

#define SMOOTH_FONT