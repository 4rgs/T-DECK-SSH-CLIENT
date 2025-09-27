#include "HardwareManager.h"
#include "../../include/config.h"

HardwareManager hardwareManager;

// Implementación de LGFX para T-Deck
LGFX::LGFX(void) {
  {
    auto cfg = _bus_instance.config();
    cfg.spi_host = SPI3_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = 27000000;
    cfg.freq_read = 16000000;
    cfg.spi_3wire = false;
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO;
    cfg.pin_sclk = BOARD_SPI_SCK;
    cfg.pin_mosi = BOARD_SPI_MOSI;
    cfg.pin_miso = BOARD_SPI_MISO;
    cfg.pin_dc = BOARD_TFT_DC;
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);
  }

  {
    auto cfg = _panel_instance.config();
    cfg.pin_cs = BOARD_TFT_CS;
    cfg.pin_rst = -1;
    cfg.pin_busy = -1;
    cfg.panel_width = 240;
    cfg.panel_height = 320;
    cfg.offset_x = 0;
    cfg.offset_y = 0;
    cfg.offset_rotation = 0;
    cfg.dummy_read_pixel = 8;
    cfg.dummy_read_bits = 1;
    cfg.readable = true;
    cfg.invert = true;
    cfg.rgb_order = true;
    cfg.dlen_16bit = false;
    cfg.bus_shared = true;
    _panel_instance.config(cfg);
  }

  {
    auto cfg = _light_instance.config();
    cfg.pin_bl = BOARD_TFT_BACKLIGHT;
    cfg.invert = true;
    cfg.freq = 44100;
    cfg.pwm_channel = 1;
    _light_instance.config(cfg);
    _panel_instance.setLight(&_light_instance);
  }

  setPanel(&_panel_instance);
}

HardwareManager::HardwareManager() {
  batteryPin = BOARD_BAT_ADC;
  backlightPin = BOARD_TFT_BACKLIGHT;
  touchIntPin = BOARD_TOUCH_INT;
  isInitialized = false;
  
  // Initialize trackball pins
  trackballPins[0] = BOARD_TRACKBALL_UP;
  trackballPins[1] = BOARD_TRACKBALL_DOWN;
  trackballPins[2] = BOARD_TRACKBALL_LEFT;
  trackballPins[3] = BOARD_TRACKBALL_RIGHT;
  trackballPins[4] = BOARD_TRACKBALL_CLICK;
  
  // Initialize states
  for (int i = 0; i < 5; i++) {
    trackballState[i] = false;
    prevTrackballState[i] = false;
  }
  
  batteryVoltage = 0.0;
  batteryPercentage = 0;
  lastBatteryRead = 0;
}

bool HardwareManager::init() {
  Serial.println("Initializing T-Deck Hardware...");
  
  // Power on sequence
  powerOn();
  delay(100);
  
  // Initialize pins
  initPins();
  
  // Initialize I2C
  initI2C();
  
  // Initialize keyboard
  keyboard.begin(KEYBOARD_I2C_ADDR);
  
  // Initialize display
  initDisplay();
  
  // Initial battery reading
  updateBatteryStatus();
  
  isInitialized = true;
  Serial.println("Hardware initialization complete");
  
  return true;
}

void HardwareManager::initDisplay() {
  display.init();
  display.setRotation(1);
  display.setBrightness(BACKLIGHT_DEFAULT);
  display.fillScreen(COLOR_BLACK);
  display.setTextColor(COLOR_WHITE);
  display.setFont(&fonts::Font0); // Mejor soporte para caracteres especiales
}

void HardwareManager::initPins() {
  // Power pin
  pinMode(BOARD_POWERON, OUTPUT);
  digitalWrite(BOARD_POWERON, HIGH);
  
  // Backlight
  pinMode(backlightPin, OUTPUT);
  
  // Trackball pins
  for (int i = 0; i < 5; i++) {
    pinMode(trackballPins[i], INPUT_PULLUP);
  }
  
  // Touch interrupt pin
  pinMode(touchIntPin, INPUT);
  
  // Battery ADC
  pinMode(batteryPin, INPUT);
}

void HardwareManager::initI2C() {
  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
  Wire.setClock(400000); // 400kHz
}

void HardwareManager::powerOn() {
  pinMode(BOARD_POWERON, OUTPUT);
  digitalWrite(BOARD_POWERON, HIGH);
  delay(100);
}

LGFX* HardwareManager::getDisplay() {
  return &display;
}

void HardwareManager::setBacklight(int brightness) {
  if (brightness < 0) brightness = 0;
  if (brightness > 255) brightness = 255;
  
  display.setBrightness(brightness);
}

int HardwareManager::getBacklight() {
  return display.getBrightness();
}

void HardwareManager::displayOn() {
  setBacklight(BACKLIGHT_DEFAULT);
}

void HardwareManager::displayOff() {
  setBacklight(0);
}

void HardwareManager::updateBatteryStatus() {
  if (millis() - lastBatteryRead < BATTERY_UPDATE_INTERVAL) {
    return;
  }
  
  // Read battery voltage
  int adcValue = analogRead(batteryPin);
  batteryVoltage = (adcValue * 3.3 * 2.0) / 4095.0; // Voltage divider
  
  // Convert to percentage (approximate)
  if (batteryVoltage >= 4.1) {
    batteryPercentage = 100;
  } else if (batteryVoltage >= 3.9) {
    batteryPercentage = 80 + (batteryVoltage - 3.9) * 100;
  } else if (batteryVoltage >= 3.7) {
    batteryPercentage = 60 + (batteryVoltage - 3.7) * 100;
  } else if (batteryVoltage >= 3.5) {
    batteryPercentage = 40 + (batteryVoltage - 3.5) * 100;
  } else if (batteryVoltage >= 3.3) {
    batteryPercentage = 20 + (batteryVoltage - 3.3) * 100;
  } else if (batteryVoltage >= 3.0) {
    batteryPercentage = (batteryVoltage - 3.0) * 66.7;
  } else {
    batteryPercentage = 0;
  }
  
  if (batteryPercentage > 100) batteryPercentage = 100;
  if (batteryPercentage < 0) batteryPercentage = 0;
  
  lastBatteryRead = millis();
}

float HardwareManager::getBatteryVoltage() {
  return batteryVoltage;
}

int HardwareManager::getBatteryPercentage() {
  return batteryPercentage;
}

bool HardwareManager::isBatteryCharging() {
  // Simplified detection - would need more sophisticated implementation
  return batteryVoltage > 4.3;
}

bool HardwareManager::isBatteryLow() {
  return batteryPercentage < BATTERY_LOW_THRESHOLD;
}

void HardwareManager::updateTrackball() {
  // Store previous states
  for (int i = 0; i < 5; i++) {
    prevTrackballState[i] = trackballState[i];
    trackballState[i] = !digitalRead(trackballPins[i]); // Active low
  }
}

bool HardwareManager::isTrackballPressed(int direction) {
  if (direction >= 0 && direction < 5) {
    return trackballState[direction];
  }
  return false;
}

bool HardwareManager::wasTrackballPressed(int direction) {
  if (direction >= 0 && direction < 5) {
    return trackballState[direction] && !prevTrackballState[direction];
  }
  return false;
}

void HardwareManager::clearTrackballState() {
  for (int i = 0; i < 5; i++) {
    trackballState[i] = false;
    prevTrackballState[i] = false;
  }
}

bool HardwareManager::isTouchPressed() {
  return !digitalRead(touchIntPin); // Active low
}

void HardwareManager::getTouchCoordinates(int& x, int& y) {
  // This would interface with the touch driver
  // For now, return default values
  x = 0;
  y = 0;
}

bool HardwareManager::isTouchInArea(int x, int y, int width, int height, int touchX, int touchY) {
  return (touchX >= x && touchX <= x + width && 
          touchY >= y && touchY <= y + height);
}

bool HardwareManager::isHardwareOK() {
  return isInitialized;
}

String HardwareManager::getHardwareInfo() {
  String info = "T-Deck Hardware Info:\n";
  info += "Display: " + String(SCREEN_WIDTH) + "x" + String(SCREEN_HEIGHT) + "\n";
  info += "Battery: " + String(batteryVoltage) + "V (" + String(batteryPercentage) + "%)\n";
  info += "Backlight: " + String(getBacklight()) + "/255\n";
  info += "Status: ";
  info += (isInitialized ? "OK" : "Error");
  
  return info;
}

void HardwareManager::reset() {
  ESP.restart();
}

void HardwareManager::deepSleep() {
  displayOff();
  esp_deep_sleep_start();
}

void HardwareManager::beep(int frequency, int duration) {
  // Implementation would require buzzer hardware
  // For now, just a placeholder
}

void HardwareManager::vibrate(int duration) {
  // Implementation would require vibration motor
  // For now, just a placeholder
}

void HardwareManager::ledOn() {
  // Implementation would control status LED if available
}

void HardwareManager::ledOff() {
  // Implementation would control status LED if available
}

void HardwareManager::ledBlink(int times, int delay_ms) {
  for (int i = 0; i < times; i++) {
    ledOn();
    delay(delay_ms);
    ledOff();
    delay(delay_ms);
  }
}

//==============================================================================
// KEYBOARD FUNCTIONS
//==============================================================================

void HardwareManager::updateKeyboard() {
  keyboard.poll();
}

bool HardwareManager::isKeyAvailable() {
  return keyboard.available();
}

uint8_t HardwareManager::readKey() {
  return keyboard.read();
}