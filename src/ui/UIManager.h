#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <LovyanGFX.hpp>
#include <vector>

// Forward declarations
class LGFX;
struct WiFiNetwork;
struct SavedWiFiNetwork;
struct SSHHost;

class UIManager {
private:
  LGFX* display;
  std::vector<String> outputLines;
  String currentInput;
  int scrollPosition;
  int maxLines;
  int cursorX;
  int cursorY;
  bool cursorVisible;
  unsigned long lastCursorBlink;
  
  // Colores del tema
  uint32_t backgroundColor;
  uint32_t textColor;
  uint32_t highlightColor;
  uint32_t promptColor;
  uint32_t errorColor;
  uint32_t successColor;
  
  // Dimensiones
  int screenWidth;
  int screenHeight;
  int lineHeight;
  int charWidth;
  
public:
  UIManager();
  void init(LGFX* displayPtr);
  
  // Gestión de salida
  void addOutput(String text);
  void clearOutput();
  void updateDisplay();
  void scrollUp();
  void scrollDown();
  
  // Gestión de entrada
  void setInput(String input);
  String getInput();
  void setCursor(int x, int y);
  void moveCursor(int deltaX, int deltaY);
  void toggleCursor();
  
  // Interfaces especializadas
  void showWiFiNetworks(const std::vector<WiFiNetwork>& networks, int selectedIndex);
  void showSavedNetworks(const std::vector<SavedWiFiNetwork>& networks);
  void showSSHHosts(const std::vector<SSHHost>& hosts, int selectedIndex);
  void showConnectionStatus();
  void showBatteryStatus();
  void showHelp();
  void showStatus();
  
  // Gestión de temas
  void setTheme(String themeName);
  void setColors(uint32_t bg, uint32_t text, uint32_t highlight, uint32_t prompt, uint32_t error, uint32_t success);
  
  // Utilidades de dibujo
  void drawProgressBar(int x, int y, int width, int height, int percentage, uint32_t color);
  void drawBox(int x, int y, int width, int height, String title, uint32_t borderColor);
  void drawCenteredText(String text, int y, uint32_t color);
  
  // Gestión de pantalla
  void setBrightness(int brightness);
  void clearScreen();
  void refresh();
  
  // Información de pantalla
  int getScreenWidth();
  int getScreenHeight();
  int getMaxLines();
  int getCurrentLine();
  int getScrollPosition();
};

extern UIManager uiManager;

#endif