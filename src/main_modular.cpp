//==============================================================================
// T-DECK SSH TERMINAL - MODULAR VERSION
// Complete SSH terminal for LilyGO T-Deck ESP32-S3
// 
// Features:
// - SSH connection with authentication
// - WiFi management with persistence  
// - Network scanning and host discovery
// - Touch interface with trackball navigation
// - Battery monitoring and system status
// - Modular architecture for easy maintenance
//
// Author: Alvaro Gonzalez
// Repository: https://github.com/4rgs/T-DECK-SSH-CLIENT
//==============================================================================

#include <Arduino.h>
#include <Wire.h>

// Hardware modules
#include "hardware/HardwareManager.h"
#include "keyboard/Keyboard.h"

// Core modules  
#include "wifi/WiFiManager.h"
#include "ssh/SSHManager.h"
#include "persistence/PersistenceManager.h"
#include "ui/UIManager.h"
#include "commands/CommandProcessor.h"

// Touch driver
#include "TouchDrvGT911.hpp"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

// Hardware
TouchDrvGT911 touchDriver;
Keyboard keyboard;

// State management
String currentInput = "";
std::vector<String> commandHistory;
int historyIndex = 0;
bool isInPasswordMode = false;
String pendingSSIDForPassword = "";

// Input handling
bool shiftPressed = false;
bool ctrlPressed = false;
int cursorX = 0;
int cursorY = 0;

// Timing
unsigned long lastUpdate = 0;
unsigned long lastKeyCheck = 0;
unsigned long lastTouchCheck = 0;
const unsigned long UPDATE_INTERVAL = 50; // 20 FPS
const unsigned long KEY_CHECK_INTERVAL = 10;
const unsigned long TOUCH_CHECK_INTERVAL = 30;

//==============================================================================
// SETUP FUNCTION
//==============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=================================");
  Serial.println("  T-DECK SSH TERMINAL v2.0");
  Serial.println("  Modular Architecture");
  Serial.println("=================================");
  
  // Initialize hardware
  if (!hardwareManager.init()) {
    Serial.println("ERROR: Hardware initialization failed!");
    return;
  }
  
  // Initialize UI
  uiManager.init(hardwareManager.getDisplay());
  uiManager.addOutput("T-Deck SSH Terminal v2.0");
  uiManager.addOutput("Modular Architecture");
  uiManager.addOutput("Initializing...");
  uiManager.updateDisplay();
  
  // Initialize touch
  touchDriver.begin();
  
  // Initialize keyboard
  if (!keyboard.begin()) {
    uiManager.addOutput("Warning: Keyboard init failed");
  }
  
  // Initialize persistence and load saved data
  persistenceManager.loadConfig("version", "2.0");
  wifiManager.loadSavedNetworks();
  sshManager.loadKnownHosts();
  
  uiManager.addOutput("Loading saved configurations...");
  uiManager.updateDisplay();
  
  // Attempt auto-connection to WiFi
  uiManager.addOutput("Attempting auto-connect...");
  uiManager.updateDisplay();
  
  if (wifiManager.attemptAutoConnect()) {
    uiManager.addOutput("Auto-connected to: " + wifiManager.getConnectedSSID());
    uiManager.addOutput("IP: " + wifiManager.getLocalIP());
  } else {
    uiManager.addOutput("Auto-connect failed or no saved networks");
    uiManager.addOutput("Use 'wifi scan' to see available networks");
  }
  
  // Show initial status
  uiManager.addOutput("");
  uiManager.addOutput("=== System Ready ===");
  uiManager.addOutput("Type 'help' for available commands");
  uiManager.addOutput("Battery: " + String(hardwareManager.getBatteryPercentage()) + "%");
  
  if (wifiManager.isConnected()) {
    uiManager.addOutput("WiFi: Connected (" + wifiManager.getConnectedSSID() + ")");
    uiManager.addOutput("Signal: " + String(wifiManager.getSignalStrength()) + " dBm");
  } else {
    uiManager.addOutput("WiFi: Disconnected");
  }
  
  uiManager.addOutput("");
  uiManager.addOutput("> ");
  uiManager.updateDisplay();
  
  Serial.println("System initialization complete");
}

//==============================================================================
// MAIN LOOP
//==============================================================================

void loop() {
  unsigned long now = millis();
  
  // Update hardware status
  if (now - lastUpdate >= UPDATE_INTERVAL) {
    hardwareManager.updateBatteryStatus();
    hardwareManager.updateTrackball();
    
    // Handle SSH data if connected
    if (sshManager.isSSHConnected()) {
      sshManager.processIncomingData();
    }
    
    lastUpdate = now;
  }
  
  // Check keyboard input
  if (now - lastKeyCheck >= KEY_CHECK_INTERVAL) {
    handleKeyboardInput();
    lastKeyCheck = now;
  }
  
  // Check touch input  
  if (now - lastTouchCheck >= TOUCH_CHECK_INTERVAL) {
    handleTouchInput();
    lastTouchCheck = now;
  }
  
  // Handle trackball navigation
  handleTrackballInput();
  
  // Update display cursor
  uiManager.toggleCursor();
  
  // Small delay to prevent excessive CPU usage
  delay(1);
}

//==============================================================================
// INPUT HANDLING FUNCTIONS
//==============================================================================

void handleKeyboardInput() {
  if (keyboard.available()) {
    KeyboardEvent event = keyboard.read();
    
    if (event.type == KEY_PRESS) {
      handleKeyPress(event);
    } else if (event.type == KEY_RELEASE) {
      handleKeyRelease(event);
    }
  }
}

void handleKeyPress(KeyboardEvent event) {
  // Handle special keys
  switch (event.key) {
    case KEY_SHIFT:
      shiftPressed = true;
      return;
      
    case KEY_CTRL:
      ctrlPressed = true;
      return;
      
    case KEY_ENTER:
      processEnterKey();
      return;
      
    case KEY_BACKSPACE:
      processBackspace();
      return;
      
    case KEY_TAB:
      processTabCompletion();
      return;
      
    case KEY_UP:
      navigateHistory(-1);
      return;
      
    case KEY_DOWN:
      navigateHistory(1);
      return;
      
    case KEY_LEFT:
      moveCursorLeft();
      return;
      
    case KEY_RIGHT:
      moveCursorRight();
      return;
      
    default:
      // Handle character input
      if (event.character != 0) {
        addCharacterToInput(event.character);
      }
      break;
  }
}

void handleKeyRelease(KeyboardEvent event) {
  switch (event.key) {
    case KEY_SHIFT:
      shiftPressed = false;
      break;
      
    case KEY_CTRL:
      ctrlPressed = false;
      break;
  }
}

void handleTouchInput() {
  if (touchDriver.available()) {
    int16_t x, y;
    if (touchDriver.read(x, y)) {
      processTouchEvent(x, y);
    }
  }
}

void handleTrackballInput() {
  // Trackball UP - scroll up
  if (hardwareManager.wasTrackballPressed(0)) {
    uiManager.scrollUp();
    uiManager.updateDisplay();
  }
  
  // Trackball DOWN - scroll down  
  if (hardwareManager.wasTrackballPressed(1)) {
    uiManager.scrollDown();
    uiManager.updateDisplay();
  }
  
  // Trackball LEFT - cursor left
  if (hardwareManager.wasTrackballPressed(2)) {
    moveCursorLeft();
  }
  
  // Trackball RIGHT - cursor right
  if (hardwareManager.wasTrackballPressed(3)) {
    moveCursorRight();
  }
  
  // Trackball CLICK - enter
  if (hardwareManager.wasTrackballPressed(4)) {
    processEnterKey();
  }
}

//==============================================================================
// INPUT PROCESSING FUNCTIONS  
//==============================================================================

void processEnterKey() {
  if (currentInput.length() == 0) {
    uiManager.addOutput("> ");
    uiManager.updateDisplay();
    return;
  }
  
  // Handle password mode
  if (isInPasswordMode) {
    processPasswordInput();
    return;
  }
  
  // Add command to history
  commandHistory.push_back(currentInput);
  if (commandHistory.size() > 20) {
    commandHistory.erase(commandHistory.begin());
  }
  historyIndex = commandHistory.size();
  
  // Echo input and process command
  uiManager.addOutput("> " + currentInput);
  
  if (commandProcessor.executeCommand(currentInput)) {
    // Command handled by processor
  } else {
    // Handle SSH session input
    if (sshManager.isSSHConnected()) {
      sshManager.sendData(currentInput + "\n");
    } else {
      uiManager.addOutput("Unknown command: " + currentInput);
      uiManager.addOutput("Type 'help' for available commands");
    }
  }
  
  currentInput = "";
  cursorX = 0;
  uiManager.setInput(currentInput);
  
  if (!sshManager.isSSHConnected()) {
    uiManager.addOutput("> ");
  }
  
  uiManager.updateDisplay();
}

void processPasswordInput() {
  String password = currentInput;
  currentInput = "";
  uiManager.setInput(""); 
  isInPasswordMode = false;
  
  uiManager.addOutput("> [password entered]");
  uiManager.addOutput("Connecting to " + pendingSSIDForPassword + "...");
  uiManager.updateDisplay();
  
  if (wifiManager.connectToNetwork(pendingSSIDForPassword, password)) {
    uiManager.addOutput("Connected successfully!");
    uiManager.addOutput("IP: " + wifiManager.getLocalIP());
    uiManager.addOutput("Signal: " + String(wifiManager.getSignalStrength()) + " dBm");
  } else {
    uiManager.addOutput("Connection failed!");
    uiManager.addOutput("Check password and try again");
  }
  
  pendingSSIDForPassword = "";
  uiManager.addOutput("> ");
  uiManager.updateDisplay();
}

void processBackspace() {
  if (currentInput.length() > 0 && cursorX > 0) {
    currentInput.remove(cursorX - 1, 1);
    cursorX--;
    uiManager.setInput(currentInput);
    uiManager.setCursor(cursorX, cursorY);
    uiManager.updateDisplay();
  }
}

void processTabCompletion() {
  String completion = commandProcessor.getTabCompletion(currentInput);
  if (completion != currentInput) {
    currentInput = completion;
    cursorX = currentInput.length();
    uiManager.setInput(currentInput);
    uiManager.setCursor(cursorX, cursorY);
    uiManager.updateDisplay();
  }
}

void navigateHistory(int direction) {
  if (commandHistory.size() == 0) return;
  
  historyIndex += direction;
  
  if (historyIndex < 0) {
    historyIndex = 0;
  } else if (historyIndex >= commandHistory.size()) {
    historyIndex = commandHistory.size();
    currentInput = "";
  } else {
    currentInput = commandHistory[historyIndex];
  }
  
  cursorX = currentInput.length();
  uiManager.setInput(currentInput);
  uiManager.setCursor(cursorX, cursorY);
  uiManager.updateDisplay();
}

void moveCursorLeft() {
  if (cursorX > 0) {
    cursorX--;
    uiManager.setCursor(cursorX, cursorY);
    uiManager.updateDisplay();
  }
}

void moveCursorRight() {
  if (cursorX < currentInput.length()) {
    cursorX++;
    uiManager.setCursor(cursorX, cursorY);
    uiManager.updateDisplay();
  }
}

void addCharacterToInput(char c) {
  // Apply shift for uppercase/symbols
  if (shiftPressed) {
    c = applyShift(c);
  }
  
  currentInput = currentInput.substring(0, cursorX) + c + currentInput.substring(cursorX);
  cursorX++;
  
  uiManager.setInput(currentInput);
  uiManager.setCursor(cursorX, cursorY);
  uiManager.updateDisplay();
}

char applyShift(char c) {
  if (c >= 'a' && c <= 'z') {
    return c - 32; // Convert to uppercase
  }
  
  // Symbol conversions
  switch (c) {
    case '1': return '!';
    case '2': return '@';
    case '3': return '#';
    case '4': return '$';
    case '5': return '%';
    case '6': return '^';
    case '7': return '&';
    case '8': return '*';
    case '9': return '(';
    case '0': return ')';
    case '-': return '_';
    case '=': return '+';
    case '[': return '{';
    case ']': return '}';
    case '\\': return '|';
    case ';': return ':';
    case '\'': return '"';
    case ',': return '<';
    case '.': return '>';
    case '/': return '?';
    case '`': return '~';
    default: return c;
  }
}

//==============================================================================
// TOUCH EVENT PROCESSING
//==============================================================================

void processTouchEvent(int x, int y) {
  // Handle touch interactions based on current screen state
  
  // If showing WiFi networks, handle network selection
  std::vector<WiFiNetwork> networks = wifiManager.getAvailableNetworks();
  if (networks.size() > 0) {
    handleWiFiNetworkTouch(x, y, networks);
    return;
  }
  
  // If showing SSH hosts, handle host selection
  std::vector<SSHHost> hosts = sshManager.getKnownHosts();
  if (hosts.size() > 0) {
    handleSSHHostTouch(x, y, hosts);
    return;
  }
  
  // Default touch handling - scroll
  handleScrollTouch(x, y);
}

void handleWiFiNetworkTouch(int x, int y, const std::vector<WiFiNetwork>& networks) {
  int lineHeight = 20;
  int startY = 60;
  
  for (int i = 0; i < networks.size(); i++) {
    int networkY = startY + (i * lineHeight);
    
    if (y >= networkY && y <= networkY + lineHeight) {
      // Network selected
      wifiManager.setSelectedNetworkIndex(i);
      
      String ssid = networks[i].ssid;
      uiManager.addOutput("Selected: " + ssid);
      
      if (networks[i].isSecure) {
        uiManager.addOutput("Enter password for " + ssid + ":");
        pendingSSIDForPassword = ssid;
        isInPasswordMode = true;
        uiManager.addOutput("> ");
      } else {
        uiManager.addOutput("Connecting to open network...");
        if (wifiManager.connectToNetwork(ssid, "")) {
          uiManager.addOutput("Connected to " + ssid);
          uiManager.addOutput("IP: " + wifiManager.getLocalIP());
        } else {
          uiManager.addOutput("Failed to connect to " + ssid);
        }
        uiManager.addOutput("> ");
      }
      
      uiManager.updateDisplay();
      return;
    }
  }
}

void handleSSHHostTouch(int x, int y, const std::vector<SSHHost>& hosts) {
  int lineHeight = 20;
  int startY = 60;
  
  for (int i = 0; i < hosts.size(); i++) {
    int hostY = startY + (i * lineHeight);
    
    if (y >= hostY && y <= hostY + lineHeight) {
      // Host selected
      sshManager.setSelectedHostIndex(i);
      
      SSHHost selectedHost = hosts[i];
      uiManager.addOutput("Selected: " + selectedHost.name);
      uiManager.addOutput("Connecting to " + selectedHost.ip + ":" + String(selectedHost.port));
      
      // For now, just show selection - full SSH implementation would require password input
      uiManager.addOutput("SSH connection: " + selectedHost.user + "@" + selectedHost.ip);
      uiManager.addOutput("> ");
      uiManager.updateDisplay();
      return;
    }
  }
}

void handleScrollTouch(int x, int y) {
  int screenHeight = hardwareManager.getDisplay()->height();
  
  if (y < screenHeight / 3) {
    // Top third - scroll up
    uiManager.scrollUp();
  } else if (y > (screenHeight * 2) / 3) {
    // Bottom third - scroll down  
    uiManager.scrollDown();
  }
  
  uiManager.updateDisplay();
}