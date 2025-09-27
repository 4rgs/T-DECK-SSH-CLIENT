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
#include <vector>

// Configuration
#include "../include/config.h"

// Hardware modules
#include "hardware/HardwareManager.h"

// Core modules  
#include "wifi/WiFiManager.h"
#include "ssh/SSHManager.h"
#include "persistence/PersistenceManager.h"
#include "ui/UIManager.h"
#include "commands/CommandProcessor.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================

// State management
String currentInput = "";
bool isInPasswordMode = false;
String pendingSSIDForPassword = "";
bool isInSSHPasswordMode = false;

// Command history
std::vector<String> commandHistory;
int historyIndex = 0;

// Input handling
bool shiftPressed = false;
bool ctrlPressed = false;
int cursorX = 0;
int cursorY = 0;

// Timing
unsigned long lastUpdate = 0;

//==============================================================================
// FUNCTION DECLARATIONS - INPUT HANDLING
//==============================================================================

// Trackball navigation functions
void handleTrackballInput();

// Keyboard input processing
void processEnterKey();
void processPasswordInput();
void addCharacterToInput(char c);
char applyShift(char c);

// Touch screen event handling
void processTouchEvent(int x, int y);
void handleWiFiNetworkTouch(int x, int y, const std::vector<WiFiNetwork>& networks);
void handleSSHHostTouch(int x, int y, const std::vector<SSHHost>& hosts);
void handleScrollTouch(int x, int y);

//==============================================================================
// SETUP FUNCTION
//==============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=================================");
  Serial.println("  T-DECK SSH TERMINAL v2.0");
  Serial.println("=================================");
  
  // Initialize hardware
  if (!hardwareManager.init()) {
    Serial.println("ERROR: Hardware initialization failed!");
    return;
  }
  
  // Initialize UI
  uiManager.init(hardwareManager.getDisplay());
  uiManager.addOutput("T-Deck SSH Terminal v2.0");
  uiManager.addOutput("Initializing...");
  uiManager.updateDisplay();
  
  // Initialize persistence and load saved data
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
  
  // Update hardware status periodically
  if (now - lastUpdate >= 50) { // 20 FPS
    hardwareManager.updateBatteryStatus();
    hardwareManager.updateTrackball();
    hardwareManager.updateKeyboard();
    
    // Process keyboard input
    while (hardwareManager.isKeyAvailable()) {
      uint8_t key = hardwareManager.readKey();
      
      if (key == 0x0A || key == 0x0D) { // Enter
        if (isInPasswordMode || isInSSHPasswordMode) {
          processPasswordInput();
        } else {
          processEnterKey();
        }
      }
      else if (key == 0x08 || key == 0x7F) { // Backspace
        if (currentInput.length() > 0) {
          currentInput.remove(currentInput.length() - 1);
          cursorX--;
          
          if (isInSSHPasswordMode) {
            // En modo password SSH, mostrar asteriscos
            String maskedInput = "";
            for (int i = 0; i < currentInput.length(); i++) {
              maskedInput += "*";
            }
            uiManager.setInput(maskedInput);
          } else {
            uiManager.setInput(currentInput);
          }
          
          uiManager.updateDisplay();
        }
      }
      else if (key >= 0x20 && key <= 0x7E) { // Printable characters
        char c = (char)key;
        addCharacterToInput(c);
      }
    }
    
    // Handle SSH data if connected or connecting
    if (sshManager.isSSHConnecting()) {
      sshManager.processIncomingData();
      
      // Detección adicional de password prompts en el output reciente
      // (Como backup si la detección en SSHManager falla)
      static unsigned long lastPasswordCheck = 0;
      if (millis() - lastPasswordCheck > 500) { // Verificar cada 500ms
        lastPasswordCheck = millis();
        
        // Verificar las últimas líneas de output para detectar password prompts
        // Esta es una detección alternativa más directa
        if (!isInSSHPasswordMode) {
          // Activar modo password manualmente si se detectan ciertos patrones
          // (Esta lógica se puede activar si es necesario)
        }
      }
    }
    
    // Handle trackball navigation
    handleTrackballInput();
    
    // Update display cursor
    uiManager.toggleCursor();
    
    lastUpdate = now;
  }
  
  // Handle serial input for testing
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() > 0) {
      Serial.println("> " + input);
      
      if (commandProcessor.executeCommand(input)) {
        // Command processed successfully
      } else {
        Serial.println("Unknown command: " + input);
      }
    }
  }
  
  // Small delay to prevent excessive CPU usage
  delay(1);
}

//==============================================================================
// INPUT HANDLING FUNCTIONS
//==============================================================================

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
  cursorX = 0;
  cursorY = 0;
  uiManager.setInput(""); 
  
  if (isInSSHPasswordMode) {
    // Enviar contraseña a SSH
    String maskedPassword = "";
    for (int i = 0; i < password.length(); i++) {
      maskedPassword += "*";
    }
    uiManager.addOutput("> " + maskedPassword);
    sshManager.sendData(password + "\n");
    isInSSHPasswordMode = false;
  } else if (isInPasswordMode) {
    // Conectar a WiFi
    isInPasswordMode = false;
    
    uiManager.addOutput("> [password entered]");
    uiManager.addOutput("Connecting to " + pendingSSIDForPassword + "...");
    
    if (wifiManager.connectToNetwork(pendingSSIDForPassword, password)) {
      uiManager.addOutput("Connected successfully!");
      uiManager.addOutput("IP: " + wifiManager.getLocalIP());
      uiManager.addOutput("Signal: " + String(wifiManager.getSignalStrength()) + " dBm");
    } else {
      uiManager.addOutput("Connection failed!");
      uiManager.addOutput("Check password and try again");
    }
    
    pendingSSIDForPassword = "";
  }
  
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
  
  if (isInSSHPasswordMode) {
    // En modo password SSH, mostrar asteriscos en lugar de caracteres reales
    String maskedInput = "";
    for (int i = 0; i < currentInput.length(); i++) {
      maskedInput += "*";
    }
    uiManager.setInput(maskedInput);
  } else {
    uiManager.setInput(currentInput);
  }
  
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