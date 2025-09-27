#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include <Arduino.h>
#include <LovyanGFX.hpp>

// Configuración de LovyanGFX para T-Deck
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;

public:
  LGFX(void);
};

class HardwareManager {
private:
  LGFX display;
  int batteryPin;
  int backlightPin;
  bool isInitialized;
  
  // Estado del trackball
  int trackballPins[5]; // UP, DOWN, LEFT, RIGHT, CLICK
  bool trackballState[5];
  bool prevTrackballState[5];
  
  // Touch
  int touchIntPin;
  
  // Batería
  float batteryVoltage;
  int batteryPercentage;
  unsigned long lastBatteryRead;
  
public:
  HardwareManager();
  
  // Inicialización
  bool init();
  void initDisplay();
  void initPins();
  void initI2C();
  void powerOn();
  
  // Display
  LGFX* getDisplay();
  void setBacklight(int brightness);
  int getBacklight();
  void displayOn();
  void displayOff();
  
  // Batería
  void updateBatteryStatus();
  float getBatteryVoltage();
  int getBatteryPercentage();
  bool isBatteryCharging();
  bool isBatteryLow();
  
  // Trackball
  void updateTrackball();
  bool isTrackballPressed(int direction); // 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT, 4=CLICK
  bool wasTrackballPressed(int direction);
  void clearTrackballState();
  
  // Touch
  bool isTouchPressed();
  void getTouchCoordinates(int& x, int& y);
  bool isTouchInArea(int x, int y, int width, int height, int touchX, int touchY);
  
  // Estado del sistema
  bool isHardwareOK();
  String getHardwareInfo();
  void reset();
  void deepSleep();
  
  // Utilidades
  void beep(int frequency, int duration);
  void vibrate(int duration);
  void ledOn();
  void ledOff();
  void ledBlink(int times, int delay_ms);
};

extern HardwareManager hardwareManager;

#endif