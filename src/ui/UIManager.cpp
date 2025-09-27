#include "UIManager.h"
#include "../../include/config.h"
#include "../wifi/WiFiManager.h"
#include "../ssh/SSHManager.h"
#include "../hardware/HardwareManager.h"

UIManager uiManager;

UIManager::UIManager() {
  display = nullptr;
  scrollPosition = 0;
  maxLines = 20;
  cursorX = 0;
  cursorY = 0;
  cursorVisible = true;
  lastCursorBlink = 0;
  
  // Colores por defecto
  backgroundColor = COLOR_BLACK;
  textColor = COLOR_WHITE;
  highlightColor = COLOR_CYAN;
  promptColor = COLOR_GREEN;
  errorColor = COLOR_RED;
  successColor = COLOR_GREEN;
  
  screenWidth = 240;
  screenHeight = 320;
  lineHeight = 16;
  charWidth = 6;
}

void UIManager::init(LGFX* displayPtr) {
  display = displayPtr;
  if (display) {
    screenWidth = display->width();
    screenHeight = display->height();
    maxLines = (screenHeight - 40) / lineHeight; // Reserve space for status
  }
}

void UIManager::addOutput(String text) {
  outputLines.push_back(text);
  
  // Limit output lines to prevent memory issues
  while (outputLines.size() > MAX_OUTPUT_LINES) {
    outputLines.erase(outputLines.begin());
  }
  
  // Auto-scroll to bottom
  if (outputLines.size() > maxLines) {
    scrollPosition = outputLines.size() - maxLines;
  }
}

void UIManager::clearOutput() {
  outputLines.clear();
  scrollPosition = 0;
}

void UIManager::updateDisplay() {
  if (!display) return;
  
  display->fillScreen(backgroundColor);
  
  // Display output lines
  int startLine = scrollPosition;
  int endLine = min((int)outputLines.size(), startLine + maxLines);
  
  for (int i = startLine; i < endLine; i++) {
    int y = (i - startLine) * lineHeight + 20;
    display->setCursor(5, y);
    display->setTextColor(textColor);
    display->print(outputLines[i]);
  }
  
  // Display input line
  if (currentInput.length() > 0 || cursorVisible) {
    int inputY = screenHeight - 20;
    display->setCursor(5, inputY);
    display->setTextColor(promptColor);
    display->print("> ");
    display->setTextColor(textColor);
    display->print(currentInput);
    
    // Draw cursor
    if (cursorVisible) {
      int cursorPosX = 5 + (2 + cursorX) * charWidth;
      display->drawLine(cursorPosX, inputY, cursorPosX, inputY + lineHeight, highlightColor);
    }
  }
  
  // Show battery and connection status
  showConnectionStatus();
  showBatteryStatus();
}

void UIManager::scrollUp() {
  if (scrollPosition > 0) {
    scrollPosition--;
  }
}

void UIManager::scrollDown() {
  int maxScroll = max(0, (int)outputLines.size() - maxLines);
  if (scrollPosition < maxScroll) {
    scrollPosition++;
  }
}

void UIManager::setInput(String input) {
  currentInput = input;
}

String UIManager::getInput() {
  return currentInput;
}

void UIManager::setCursor(int x, int y) {
  cursorX = x;
  cursorY = y;
}

void UIManager::moveCursor(int deltaX, int deltaY) {
  cursorX += deltaX;
  cursorY += deltaY;
  
  // Bounds checking
  if (cursorX < 0) cursorX = 0;
  if (cursorX > currentInput.length()) cursorX = currentInput.length();
}

void UIManager::toggleCursor() {
  if (millis() - lastCursorBlink > CURSOR_BLINK_RATE) {
    cursorVisible = !cursorVisible;
    lastCursorBlink = millis();
  }
}

void UIManager::showWiFiNetworks(const std::vector<WiFiNetwork>& networks, int selectedIndex) {
  if (!display) return;
  
  addOutput("=== Available WiFi Networks ===");
  
  for (int i = 0; i < networks.size(); i++) {
    String line = "";
    if (i == selectedIndex) {
      line += "> ";
    } else {
      line += "  ";
    }
    
    line += networks[i].ssid;
    line += " (";
    line += String(networks[i].rssi);
    line += " dBm)";
    
    if (networks[i].isSecure) {
      line += " [SECURE]";
    }
    
    addOutput(line);
  }
  
  addOutput("Use UP/DOWN to select, ENTER to connect");
}

void UIManager::showSavedNetworks(const std::vector<SavedWiFiNetwork>& networks) {
  if (networks.size() == 0) {
    addOutput("No saved networks found");
    return;
  }
  
  addOutput("=== Saved WiFi Networks ===");
  
  for (int i = 0; i < networks.size(); i++) {
    String line = "  ";
    line += networks[i].ssid;
    line += " (Last: ";
    line += String((millis() - networks[i].lastConnected) / 60000);
    line += "min ago)";
    
    if (networks[i].autoConnect) {
      line += " [AUTO]";
    }
    
    addOutput(line);
  }
}

void UIManager::showSSHHosts(const std::vector<SSHHost>& hosts, int selectedIndex) {
  if (!display) return;
  
  addOutput("=== Available SSH Hosts ===");
  
  for (int i = 0; i < hosts.size(); i++) {
    String line = "";
    if (i == selectedIndex) {
      line += "> ";
    } else {
      line += "  ";
    }
    
    line += hosts[i].name;
    line += " (";
    line += hosts[i].ip;
    line += ":";
    line += String(hosts[i].port);
    line += ")";
    
    if (hosts[i].isAvailable) {
      line += " [ONLINE]";
    } else {
      line += " [OFFLINE]";
    }
    
    addOutput(line);
  }
  
  addOutput("Use UP/DOWN to select, ENTER to connect");
}

void UIManager::showConnectionStatus() {
  if (!display) return;
  
  // Show WiFi status in top-right
  String wifiStatus = "WiFi: ";
  if (wifiManager.isConnected()) {
    wifiStatus += "OK";
    display->setTextColor(successColor);
  } else {
    wifiStatus += "OFF";
    display->setTextColor(errorColor);
  }
  
  display->setCursor(screenWidth - 60, 5);
  display->print(wifiStatus);
}

void UIManager::showBatteryStatus() {
  if (!display) return;
  
  // Show battery in top-left
  int batteryPercent = hardwareManager.getBatteryPercentage();
  
  uint32_t batteryColor = successColor;
  if (batteryPercent < 20) {
    batteryColor = errorColor;
  } else if (batteryPercent < 50) {
    batteryColor = COLOR_YELLOW;
  }
  
  display->setTextColor(batteryColor);
  display->setCursor(5, 5);
  display->print("Bat: " + String(batteryPercent) + "%");
}

void UIManager::showHelp() {
  addOutput("=== T-Deck SSH Terminal Commands ===");
  addOutput("");
  addOutput("WiFi Commands:");
  addOutput("  wifi scan        - Scan for networks");
  addOutput("  wifi connect <ssid> - Connect to network");
  addOutput("  wifi disconnect  - Disconnect from network");
  addOutput("  wifi status      - Show connection status");
  addOutput("  saved            - List saved networks");
  addOutput("  forget <ssid>    - Remove saved network");
  addOutput("");
  addOutput("SSH Commands:");
  addOutput("  ssh <user@host>  - Connect via SSH");
  addOutput("  hosts            - List known SSH hosts");
  addOutput("  scan             - Scan network for SSH hosts");
  addOutput("");
  addOutput("General Commands:");
  addOutput("  help             - Show this help");
  addOutput("  clear            - Clear screen");
  addOutput("  status           - System status");
  addOutput("  exit             - Disconnect SSH session");
}

void UIManager::showStatus() {
  addOutput("=== System Status ===");
  addOutput("Firmware: v" + String(FIRMWARE_VERSION));
  addOutput("Battery: " + String(hardwareManager.getBatteryPercentage()) + "%");
  
  if (wifiManager.isConnected()) {
    addOutput("WiFi: Connected to " + wifiManager.getConnectedSSID());
    addOutput("IP: " + wifiManager.getLocalIP());
    addOutput("Signal: " + String(wifiManager.getSignalStrength()) + " dBm");
  } else {
    addOutput("WiFi: Disconnected");
  }
  
  if (sshManager.isSSHConnected()) {
    addOutput("SSH: Connected to " + sshManager.getCurrentHost());
  } else {
    addOutput("SSH: Not connected");
  }
}

void UIManager::setTheme(String themeName) {
  if (themeName == "dark") {
    backgroundColor = COLOR_BLACK;
    textColor = COLOR_WHITE;
    highlightColor = COLOR_CYAN;
    promptColor = COLOR_GREEN;
    errorColor = COLOR_RED;
    successColor = COLOR_GREEN;
  } else if (themeName == "light") {
    backgroundColor = COLOR_WHITE;
    textColor = COLOR_BLACK;
    highlightColor = COLOR_BLUE;
    promptColor = COLOR_BLUE;
    errorColor = COLOR_RED;
    successColor = COLOR_GREEN;
  }
}

void UIManager::setColors(uint32_t bg, uint32_t text, uint32_t highlight, uint32_t prompt, uint32_t error, uint32_t success) {
  backgroundColor = bg;
  textColor = text;
  highlightColor = highlight;
  promptColor = prompt;
  errorColor = error;
  successColor = success;
}

void UIManager::drawProgressBar(int x, int y, int width, int height, int percentage, uint32_t color) {
  if (!display) return;
  
  display->drawRect(x, y, width, height, color);
  int fillWidth = (percentage * (width - 2)) / 100;
  display->fillRect(x + 1, y + 1, fillWidth, height - 2, color);
}

void UIManager::drawBox(int x, int y, int width, int height, String title, uint32_t borderColor) {
  if (!display) return;
  
  display->drawRect(x, y, width, height, borderColor);
  if (title.length() > 0) {
    display->setCursor(x + 5, y + 5);
    display->setTextColor(borderColor);
    display->print(title);
  }
}

void UIManager::drawCenteredText(String text, int y, uint32_t color) {
  if (!display) return;
  
  int x = (screenWidth - text.length() * charWidth) / 2;
  display->setCursor(x, y);
  display->setTextColor(color);
  display->print(text);
}

void UIManager::setBrightness(int brightness) {
  // Implementation depends on hardware manager
  hardwareManager.setBacklight(brightness);
}

void UIManager::clearScreen() {
  if (!display) return;
  display->fillScreen(backgroundColor);
}

void UIManager::refresh() {
  updateDisplay();
}

int UIManager::getScreenWidth() {
  return screenWidth;
}

int UIManager::getScreenHeight() {
  return screenHeight;
}

int UIManager::getMaxLines() {
  return maxLines;
}

int UIManager::getCurrentLine() {
  return outputLines.size();
}

int UIManager::getScrollPosition() {
  return scrollPosition;
}