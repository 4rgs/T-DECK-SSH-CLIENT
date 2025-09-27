#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <vector>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "keyboard/Keyboard.h"
#include "../include/config.h"
#include "TouchDrvGT911.hpp"

// Pines del T-Deck según la documentación oficial
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

// Configuración de LovyanGFX para T-Deck
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI _bus_instance;
  lgfx::Light_PWM _light_instance;

public:
  LGFX(void) {
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
};

// Estructura para hosts SSH
struct SSHHost {
  String name;
  String ip;
  int port;
  String user;
  bool isAvailable;
  unsigned long lastSeen;
};

// Estructura para redes WiFi conocidas
struct SavedWiFiNetwork {
  String ssid;
  String password;
  int rssi;
  unsigned long lastConnected;
  bool autoConnect;
};

// Variables globales
LGFX display;
Keyboard keyboard;
TouchDrvGT911 touch;
String inputText = "";
int outputLine = 55;
WiFiClient sshClient;
bool sshConnected = false;

// Variables para touch y trackball
int16_t touchX = 0, touchY = 0;
bool touchPressed = false;
volatile bool trackballUpPressed = false;
volatile bool trackballDownPressed = false;
volatile bool trackballLeftPressed = false;
volatile bool trackballRightPressed = false;
volatile bool trackballClickPressed = false;

// Terminal buffer y scroll
std::vector<String> terminalBuffer;
int scrollOffset = 0;
String sshBuffer = "";

// Variables de colores (se inicializarán después del display)
uint16_t TERMINAL_BLACK;
uint16_t TERMINAL_WHITE;
uint16_t TERMINAL_GREEN;
uint16_t TERMINAL_RED;
uint16_t TERMINAL_BLUE;
uint16_t TERMINAL_YELLOW;
uint16_t TERMINAL_CYAN;
uint16_t TERMINAL_MAGENTA;
uint16_t TERMINAL_GRAY;

// Variables para descubrimiento de red
std::vector<SSHHost> knownHosts;
std::vector<String> discoveredIPs;
bool inHostSelection = false;
int selectedHostIndex = 0;
bool scanningNetwork = false;
unsigned long lastNetworkScan = 0;
const unsigned long NETWORK_SCAN_INTERVAL = 30000; // 30 segundos

// Variables para batería
float batteryVoltage = 0.0;
int batteryPercent = 0;
unsigned long lastBatteryUpdate = 0;

// Variables para WiFi scanning
std::vector<String> availableNetworks;
int selectedNetwork = 0;
bool inWifiSetup = false;
String currentSSID = "";
String wifiPassword = "";
bool enteringPassword = false;

// Variables para persistencia
Preferences preferences;
std::vector<SavedWiFiNetwork> savedNetworks;
bool autoConnectAttempted = false;

// Declaraciones de funciones
void updateDisplay();
void executeCommand(String cmd);
void addOutput(String text);
void connectSSH();
void scanWiFiNetworks();
void displayWiFiSetup();
void connectToSelectedWiFi();
void disconnectWiFi();
void updateBatteryInfo();
void drawBatteryInfo();
void handleTouchTap(int16_t x, int16_t y);
void disconnectSSH();
void handleSSHData();
void sendSSHCommand(String cmd);
void initializeKnownHosts();
void scanNetworkForSSH();
bool isPortOpen(String ip, int port, int timeout);
void displayHostSelection();
void selectSSHHost();
void connectToHost(int hostIndex);
String cleanANSI(String input);
void redrawTerminal();
void handleTouch();
void loadSavedNetworks();
void saveSavedNetworks();
void saveWiFiCredentials(String ssid, String password);
void loadSSHHosts();
void saveSSHHosts();
bool attemptAutoConnect();
void handleTrackball();
void scrollUp();
void scrollDown();
void sendTabCommand();

// Funciones de interrupción para trackball
void IRAM_ATTR trackballUpISR() { trackballUpPressed = true; }
void IRAM_ATTR trackballDownISR() { trackballDownPressed = true; }
void IRAM_ATTR trackballLeftISR() { trackballLeftPressed = true; }
void IRAM_ATTR trackballRightISR() { trackballRightPressed = true; }
void IRAM_ATTR trackballClickISR() { trackballClickPressed = true; }

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("T-Deck SSH Terminal Starting...");
  
  // Encender periféricos
  pinMode(BOARD_POWERON, OUTPUT);
  digitalWrite(BOARD_POWERON, HIGH);
  delay(1000);
  
  // Inicializar I2C y teclado
  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
  Wire.setClock(100000);
  keyboard.begin(KEYBOARD_I2C_ADDR);
  
  // Configurar ADC para lectura de batería
  pinMode(BOARD_BAT_ADC, INPUT);
  analogReadResolution(12); // 12-bit ADC para mayor precisión
  
  // Inicializar touch GT911
  pinMode(BOARD_TOUCH_INT, INPUT);
  touch.setPins(-1, BOARD_TOUCH_INT);
  if (touch.begin(Wire, GT911_SLAVE_ADDRESS_L)) {
    Serial.println("Touch GT911 initialized successfully");
    touch.setMaxCoordinates(320, 240);
    touch.setSwapXY(true);
    touch.setMirrorXY(false, true);
  } else {
    Serial.println("Touch GT911 initialization failed");
  }
  
  // Configurar trackball con interrupciones
  pinMode(BOARD_TRACKBALL_UP, INPUT_PULLUP);
  pinMode(BOARD_TRACKBALL_DOWN, INPUT_PULLUP);
  pinMode(BOARD_TRACKBALL_LEFT, INPUT_PULLUP);
  pinMode(BOARD_TRACKBALL_RIGHT, INPUT_PULLUP);
  pinMode(BOARD_TRACKBALL_CLICK, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(BOARD_TRACKBALL_UP), trackballUpISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BOARD_TRACKBALL_DOWN), trackballDownISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BOARD_TRACKBALL_LEFT), trackballLeftISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BOARD_TRACKBALL_RIGHT), trackballRightISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BOARD_TRACKBALL_CLICK), trackballClickISR, FALLING);
  
  // Inicializar display
  display.init();
  display.setRotation(1);
  
  // CRUCIAL: Configurar profundidad de color ANTES de usar colores
  display.setColorDepth(16);  // RGB565 16-bit para ST7789
  
  // Inicializar colores usando las funciones correctas de LovyanGFX
  TERMINAL_BLACK   = display.color565(0, 0, 0);       // Negro
  TERMINAL_WHITE   = display.color565(255, 255, 255); // Blanco
  TERMINAL_GREEN   = display.color565(0, 255, 0);     // Verde
  TERMINAL_RED     = display.color565(255, 0, 0);     // Rojo
  TERMINAL_BLUE    = display.color565(0, 0, 255);     // Azul
  TERMINAL_YELLOW  = display.color565(255, 255, 0);   // Amarillo
  TERMINAL_CYAN    = display.color565(0, 255, 255);   // Cian
  TERMINAL_MAGENTA = display.color565(255, 0, 255);   // Magenta
  TERMINAL_GRAY    = display.color565(128, 128, 128); // Gris
  
  display.fillScreen(TERMINAL_BLACK);
  
  // Header más compacto
  display.setFont(&fonts::Font2);
  display.setTextColor(TERMINAL_GREEN, TERMINAL_BLACK);
  display.setCursor(7, 8);
  display.println("T-Deck SSH v1.3");
  
  // Líneas separadoras
  display.drawLine(0, 22, 320, 22, TERMINAL_BLUE);
  display.drawLine(0, 215, 320, 215, TERMINAL_BLUE);
  
  // Inicializar y mostrar batería
  updateBatteryInfo();
  drawBatteryInfo();
  
  // Inicializar hosts conocidos y cargar datos persistentes
  initializeKnownHosts();
  loadSavedNetworks();
  loadSSHHosts();
  
  // Inicializar WiFi y mostrar configuración
  addOutput("Starting terminal...");
  addOutput("SSH Host Discovery enabled");
  
  // Intentar conexión automática antes de escanear
  if (attemptAutoConnect()) {
    addOutput("Auto-connected to saved network");
    addOutput("Type 'hosts' to select SSH host");
  } else {
    addOutput("Scanning WiFi networks...");
    inWifiSetup = true;
    scanWiFiNetworks();
  }
  
  inWifiSetup = true;
  scanWiFiNetworks();
  
  updateDisplay();
}

void loop() {
  handleSSHData();
  keyboard.poll();
  handleTouch();
  handleTrackball();
  
  // Actualizar info de batería cada 30 segundos
  if (millis() - lastBatteryUpdate > 30000) {
    updateBatteryInfo();
    drawBatteryInfo();
    lastBatteryUpdate = millis();
  }
  
  // Escaneo periódico de red para SSH (solo si WiFi conectado y no en setup)
  if (WiFi.status() == WL_CONNECTED && !inWifiSetup && 
      millis() - lastNetworkScan > NETWORK_SCAN_INTERVAL && !scanningNetwork) {
    scanNetworkForSSH();
    lastNetworkScan = millis();
  }
  
  while (keyboard.available()) {
    uint8_t key = keyboard.read();
    
    if (inHostSelection) {
      // Manejo especial durante selección de host SSH
      if (key == 0x0A || key == 0x0D) { // Enter
        connectToHost(selectedHostIndex);
        inHostSelection = false;
        updateDisplay();
      }
      else if (key == 0x1B) { // ESC - salir de selección
        inHostSelection = false;
        addOutput("Host selection cancelled");
        updateDisplay();
      }
    }
    else if (inWifiSetup) {
      // Manejo especial durante configuración WiFi
      if (key == 0x0A || key == 0x0D) { // Enter
        if (enteringPassword) {
          wifiPassword = inputText;
          connectToSelectedWiFi();
          inputText = "";
        } else {
          currentSSID = availableNetworks[selectedNetwork];
          addOutput("Enter password for: " + currentSSID);
          enteringPassword = true;
          inputText = "";
        }
        updateDisplay();
      }
      else if (key == 0x08 || key == 0x7F) { // Backspace
        if (inputText.length() > 0) {
          inputText = inputText.substring(0, inputText.length() - 1);
          updateDisplay();
        }
      }
      else if (key >= 32 && key <= 126) { // Caracteres imprimibles
        if (enteringPassword) {
          inputText += (char)key;
          updateDisplay();
        }
      }
    }
    else {
      // Manejo normal del terminal
      if (key == 0x12) { // Tecla ALT - enviar ESC
        if (sshConnected && sshClient.connected()) {
          sshClient.write(0x1B); // Enviar código ESC (0x1B)
        }
      }
      else if (key == 0x08 || key == 0x7F) { // Backspace
        if (inputText.length() > 0) {
          inputText = inputText.substring(0, inputText.length() - 1);
          updateDisplay();
        }
      }
      else if (key == 0x0A || key == 0x0D) { // Enter
        if (sshConnected) {
          sendSSHCommand(inputText);
        } else {
          executeCommand(inputText);
        }
        inputText = "";
        updateDisplay();
      }
      else if (key >= 32 && key <= 126) { // Caracteres imprimibles
        inputText += (char)key;
        updateDisplay();
      }
    }
  }
  
  delay(10);
}

// ===== FUNCIONES NUEVAS PARA DESCUBRIMIENTO SSH =====

void initializeKnownHosts() {
  knownHosts.clear();
  
  // Agregar hosts conocidos por defecto
  SSHHost defaultHost;
  defaultHost.name = "Default Bridge";
  defaultHost.ip = String(BRIDGE_HOST);
  defaultHost.port = BRIDGE_PORT;
  defaultHost.user = "user";
  defaultHost.isAvailable = false;
  defaultHost.lastSeen = 0;
  knownHosts.push_back(defaultHost);
  
  // Hosts comunes en redes domésticas
  SSHHost routerHost;
  routerHost.name = "Router/Gateway";
  routerHost.ip = "192.168.1.1";
  routerHost.port = 22;
  routerHost.user = "admin";
  routerHost.isAvailable = false;
  routerHost.lastSeen = 0;
  knownHosts.push_back(routerHost);
  
  SSHHost piHost;
  piHost.name = "Raspberry Pi";
  piHost.ip = "192.168.1.100";
  piHost.port = 22;
  piHost.user = "pi";
  piHost.isAvailable = false;
  piHost.lastSeen = 0;
  knownHosts.push_back(piHost);
  
  Serial.println("SSH hosts initialized: " + String(knownHosts.size()) + " hosts");
}

void scanNetworkForSSH() {
  if (scanningNetwork) return;
  
  scanningNetwork = true;
  addOutput("Scanning local network...");
  
  // Obtener IP local y rango de red
  IPAddress localIP = WiFi.localIP();
  String networkBase = String(localIP[0]) + "." + String(localIP[1]) + "." + String(localIP[2]) + ".";
  
  Serial.println("Network scan started on: " + networkBase + "x");
  
  // Escanear rango común de IPs domésticas (rango limitado para no bloquear)
  for (int i = 1; i <= 50; i++) { // Reducido a 50 para ser más rápido
    String testIP = networkBase + String(i);
    
    // Verificar puerto SSH (22)
    if (isPortOpen(testIP, 22, 300)) {
      // Buscar si ya existe este host
      bool hostExists = false;
      for (int j = 0; j < knownHosts.size(); j++) {
        if (knownHosts[j].ip == testIP && knownHosts[j].port == 22) {
          knownHosts[j].isAvailable = true;
          knownHosts[j].lastSeen = millis();
          hostExists = true;
          break;
        }
      }
      
      // Si no existe, agregarlo
      if (!hostExists) {
        SSHHost newHost;
        newHost.name = "SSH-" + testIP;
        newHost.ip = testIP;
        newHost.port = 22;
        newHost.user = "user";
        newHost.isAvailable = true;
        newHost.lastSeen = millis();
        knownHosts.push_back(newHost);
        
        Serial.println("New SSH host found: " + testIP);
      }
    }
    
    // Pequeña pausa para no sobrecargar la red
    if (i % 5 == 0) {
      delay(10);
      // Procesar eventos mientras escaneamos
      keyboard.poll();
      handleSSHData();
    }
  }
  
  addOutput("Network scan complete!");
  addOutput("Found " + String(knownHosts.size()) + " SSH hosts");
  addOutput("Use 'hosts' to select host");
  
  scanningNetwork = false;
}

bool isPortOpen(String ip, int port, int timeout) {
  WiFiClient testClient;
  testClient.setTimeout(timeout);
  
  bool result = testClient.connect(ip.c_str(), port);
  if (result) {
    testClient.stop();
  }
  
  return result;
}

void selectSSHHost() {
  if (knownHosts.size() == 0) {
    addOutput("No SSH hosts available");
    addOutput("Use 'scan' to discover hosts");
    return;
  }
  
  inHostSelection = true;
  selectedHostIndex = 0;
  displayHostSelection();
}

void displayHostSelection() {
  terminalBuffer.clear();
  addOutput("SSH Hosts (tap or use trackball):");
  addOutput("Press ENTER to connect, ESC to cancel");
  addOutput("");
  
  for (int i = 0; i < knownHosts.size(); i++) {
    String prefix = (i == selectedHostIndex) ? "[*] " : "[ ] ";
    String status = knownHosts[i].isAvailable ? " [ONLINE]" : " [?]";
    String hostInfo = knownHosts[i].name + " - " + knownHosts[i].ip + ":" + String(knownHosts[i].port);
    
    // Truncar si es muy largo
    if (hostInfo.length() > 35) {
      hostInfo = hostInfo.substring(0, 32) + "...";
    }
    
    addOutput(prefix + hostInfo + status);
  }
  
  addOutput("");
  addOutput("Selected: " + knownHosts[selectedHostIndex].name);
  addOutput("User: " + knownHosts[selectedHostIndex].user + "@" + knownHosts[selectedHostIndex].ip);
  
  redrawTerminal();
}

void connectToHost(int hostIndex) {
  if (hostIndex < 0 || hostIndex >= knownHosts.size()) {
    addOutput("Invalid host selection");
    return;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    addOutput("Error: WiFi not connected");
    return;
  }
  
  SSHHost& selectedHost = knownHosts[hostIndex];
  
  addOutput("Connecting to " + selectedHost.name);
  addOutput(selectedHost.user + "@" + selectedHost.ip + ":" + String(selectedHost.port));
  
  sshClient.setTimeout(5000);
  
  if (sshClient.connect(selectedHost.ip.c_str(), selectedHost.port)) {
    sshConnected = true;
    sshBuffer = "";
    
    addOutput("SSH Connected to " + selectedHost.name + "!");
    addOutput("Remote terminal active");
    addOutput("Type 'exit' to disconnect");
    
    // Marcar como disponible y actualizar última vez visto
    selectedHost.isAvailable = true;
    selectedHost.lastSeen = millis();
    
    delay(500);
    sshClient.println("echo 'T-Deck connected to " + selectedHost.name + "'");
  } else {
    addOutput("Connection FAILED!");
    addOutput("Check host: " + selectedHost.ip + ":" + String(selectedHost.port));
    
    // Marcar como no disponible
    selectedHost.isAvailable = false;
  }
}

// ===== FUNCIONES PRINCIPALES (CONTINUACIÓN) =====

void executeCommand(String cmd) {
  if (sshConnected) return;
  
  addOutput("user@t-deck:~$ " + cmd);
  
  if (cmd == "ssh") {
    connectSSH();
  }
  else if (cmd == "hosts") {
    selectSSHHost();
  }
  else if (cmd == "scan") {
    if (WiFi.status() != WL_CONNECTED) {
      addOutput("Error: WiFi not connected");
      return;
    }
    addOutput("Scanning network for SSH hosts...");
    scanNetworkForSSH();
  }
  else if (cmd == "saved") {
    // Mostrar redes WiFi guardadas
    if (savedNetworks.size() == 0) {
      addOutput("No saved networks");
    } else {
      addOutput("Saved WiFi networks:");
      for (int i = 0; i < savedNetworks.size(); i++) {
        String status = (WiFi.SSID() == savedNetworks[i].ssid) ? " [CURRENT]" : "";
        String autoConn = savedNetworks[i].autoConnect ? " [AUTO]" : "";
        addOutput("  " + savedNetworks[i].ssid + status + autoConn);
      }
    }
  }
  else if (cmd == "forget") {
    // Olvidar red actual
    if (WiFi.status() == WL_CONNECTED) {
      String currentNetwork = WiFi.SSID();
      for (int i = 0; i < savedNetworks.size(); i++) {
        if (savedNetworks[i].ssid == currentNetwork) {
          savedNetworks.erase(savedNetworks.begin() + i);
          saveSavedNetworks();
          addOutput("Forgot network: " + currentNetwork);
          WiFi.disconnect();
          addOutput("Disconnected from WiFi");
          return;
        }
      }
      addOutput("Current network not in saved list");
    } else {
      addOutput("Not connected to any network");
    }
  }
  else if (cmd == "help") {
    addOutput("T-Deck SSH Terminal v1.3");
    addOutput("Commands:");
    addOutput("  ssh    - Connect to default host");
    addOutput("  hosts  - Select SSH host");
    addOutput("  scan   - Scan network for SSH");
    addOutput("  wifi   - WiFi setup/status");
    addOutput("  saved  - Show saved networks");
    addOutput("  forget - Forget current network");
    addOutput("  disconnect - Disconnect WiFi");
    addOutput("  clear  - Clear terminal");
    addOutput("Default: " + String(BRIDGE_HOST) + ":" + String(BRIDGE_PORT));
  }
  else if (cmd == "wifi") {
    if (WiFi.status() == WL_CONNECTED) {
      addOutput("WiFi: Connected");
      addOutput("IP: " + WiFi.localIP().toString());
      addOutput("SSID: " + WiFi.SSID());
    } else {
      addOutput("WiFi: Not connected");
      addOutput("Starting WiFi setup...");
      inWifiSetup = true;
      scanWiFiNetworks();
    }
  }
  else if (cmd == "disconnect") {
    disconnectWiFi();
  }
  else if (cmd == "clear") {
    terminalBuffer.clear();
    scrollOffset = 0;
    redrawTerminal();
  }
  else if (cmd != "") {
    addOutput("Unknown: " + cmd);
    addOutput("Type 'help' for commands");
  }
}

void updateDisplay() {
  // Limpiar COMPLETAMENTE el área del input
  display.fillRect(0, 216, 320, 24, TERMINAL_BLACK);
  
  display.setFont(&fonts::Font2);
  uint16_t textColor = sshConnected ? TERMINAL_YELLOW : TERMINAL_CYAN;
  display.setTextColor(textColor, TERMINAL_BLACK);
  
  // Mostrar prompt
  display.setCursor(7, 225);
  if (sshConnected) {
    display.setTextColor(TERMINAL_RED, TERMINAL_BLACK);
    display.print("SSH> ");
  } else if (enteringPassword) {
    display.setTextColor(TERMINAL_YELLOW, TERMINAL_BLACK);
    display.print("Pass> ");
  } else if (inWifiSetup) {
    display.setTextColor(TERMINAL_MAGENTA, TERMINAL_BLACK);
    display.print("WiFi> ");
  } else {
    display.setTextColor(TERMINAL_CYAN, TERMINAL_BLACK);
    display.print("t-deck> ");
  }
  
  // Input con mejor control de longitud
  int promptWidth = sshConnected ? 48 : (enteringPassword || inWifiSetup) ? 48 : 64;
  String displayInput = inputText;
  int maxInputChars = 35;
  
  if (displayInput.length() > maxInputChars) {
    displayInput = displayInput.substring(displayInput.length() - maxInputChars + 1);
  }
  
  display.setCursor(promptWidth + 7, 225);
  display.setTextColor(TERMINAL_WHITE, TERMINAL_BLACK);
  
  // Mostrar asteriscos si está ingresando contraseña
  if (enteringPassword) {
    String maskedInput = "";
    for (int i = 0; i < displayInput.length(); i++) {
      maskedInput += "*";
    }
    display.print(maskedInput);
  } else {
    display.print(displayInput);
  }
  
  // Cursor
  int cursorX = promptWidth + 7 + displayInput.length() * 6;
  if (cursorX < 310) {
    display.fillRect(cursorX, 222, 2, 10, TERMINAL_YELLOW);
  }
}

void addOutput(String text) {
  const int maxChars = 45;
  
  if (text.length() <= maxChars) {
    terminalBuffer.push_back(text);
  } else {
    int startPos = 0;
    while (startPos < text.length()) {
      int endPos = min(startPos + maxChars, (int)text.length());
      
      if (endPos < text.length()) {
        int lastSpace = text.lastIndexOf(' ', endPos);
        if (lastSpace > startPos) {
          endPos = lastSpace;
        }
      }
      
      String chunk = text.substring(startPos, endPos);
      chunk.trim();
      if (chunk.length() > 0) {
        terminalBuffer.push_back(chunk);
      }
      startPos = endPos + 1;
    }
  }
  
  while (terminalBuffer.size() > 100) {
    terminalBuffer.erase(terminalBuffer.begin());
  }
  
  scrollOffset = max(0, (int)terminalBuffer.size() - 12);
  redrawTerminal();
}

void redrawTerminal() {
  display.fillRect(0, 23, 320, 192, TERMINAL_BLACK);
  display.setFont(&fonts::Font2);
  
  int y = 30;
  int lineHeight = 16;
  int maxLines = 11;
  
  for (int i = scrollOffset; i < terminalBuffer.size() && i < scrollOffset + maxLines; i++) {
    if (y > 200) break;
    
    display.fillRect(0, y-2, 320, lineHeight+2, TERMINAL_BLACK);
    display.setCursor(7, y);
    display.setTextColor(TERMINAL_GREEN, TERMINAL_BLACK);
    display.print(terminalBuffer[i]);
    y += lineHeight;
  }
}

void connectSSH() {
  if (WiFi.status() != WL_CONNECTED) {
    addOutput("Error: WiFi not connected");
    return;
  }
  
  addOutput("Connecting to " + String(BRIDGE_HOST) + ":" + String(BRIDGE_PORT));
  
  sshClient.setTimeout(5000);
  
  if (sshClient.connect(BRIDGE_HOST, BRIDGE_PORT)) {
    sshConnected = true;
    sshBuffer = "";
    
    addOutput("SSH Connected!");
    addOutput("Remote terminal active");
    addOutput("Type 'exit' to disconnect");
    
    delay(500);
    sshClient.println("echo 'T-Deck connected'");
  } else {
    addOutput("Connection FAILED!");
    addOutput("Check server: " + String(BRIDGE_HOST));
  }
}

void disconnectSSH() {
  if (sshConnected) {
    sshClient.stop();
    sshConnected = false;
    addOutput("SSH Disconnected");
  }
}

void handleSSHData() {
  if (!sshConnected) return;
  
  if (!sshClient.connected()) {
    addOutput("Connection lost");
    disconnectSSH();
    return;
  }
  
  String rawData = "";
  
  while (sshClient.available()) {
    char c = sshClient.read();
    rawData += c;
    
    if (c == '\n' || rawData.length() > 80) {
      String cleanedData = cleanANSI(rawData);
      cleanedData.trim();
      
      if (cleanedData.length() > 0) {
        if (cleanedData.indexOf("  ") != -1 && cleanedData.length() > 20) {
          addOutput("[TAB]" + cleanedData);
        } else {
          addOutput(cleanedData);
        }
      }
      
      rawData = "";
    }
  }
  
  if (rawData.length() > 0) {
    String cleanedData = cleanANSI(rawData);
    cleanedData.trim();
    if (cleanedData.length() > 0) {
      sshBuffer += cleanedData;
      
      if (sshBuffer.length() > 40) {
        addOutput(sshBuffer);
        sshBuffer = "";
      }
    }
  }
}

void sendSSHCommand(String cmd) {
  if (!sshConnected || !sshClient.connected()) {
    addOutput("No SSH connection");
    return;
  }
  
  if (cmd == "exit" || cmd == "logout" || cmd == "disconnect") {
    disconnectSSH();
    return;
  }
  
  sshClient.println(cmd);
  sshClient.flush();
}

String cleanANSI(String input) {
  String result = "";
  bool inEscape = false;
  bool inCSI = false;
  bool inOSC = false;
  
  for (int i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
    
    if (c == ']' && !inEscape) {
      inOSC = true;
      continue;
    }
    
    if (inOSC) {
      if (c == 7 || (c == '\\' && i > 0 && input.charAt(i-1) == '\033')) {
        inOSC = false;
      }
      continue;
    }
    
    if (c == 27 || c == '\033') {
      inEscape = true;
      continue;
    }
    
    if (inEscape && c == '[') {
      inCSI = true;
      inEscape = false;
      continue;
    }
    
    if (inCSI) {
      if ((c >= '@' && c <= '~') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        inCSI = false;
      }
      continue;
    }
    
    if (inEscape) {
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '~' || c == 'm') {
        inEscape = false;
      }
      continue;
    }
    
    if (c < 32 && c != '\n' && c != '\t') continue;
    if (c == 127 || c == '\r') continue;
    if ((unsigned char)c == 0xEF || (unsigned char)c == 0xBF || (unsigned char)c == 0xBD) continue; // UTF-8 replacement char
    
    String substr = input.substring(i);
    if (substr.startsWith("@casaos:") || substr.startsWith("pids@")) {
      while (i < input.length() && input.charAt(i) != '$' && input.charAt(i) != '\n') {
        i++;
      }
      if (i < input.length() && input.charAt(i) == '$') {
        i++;
      }
      continue;
    }
    
    result += c;
  }
  
  return result;
}

void handleTouch() {
  int16_t x[5], y[5];
  uint8_t touchCount = touch.getPoint(x, y, 5);
  
  static bool wasTouchPressed = false;
  static unsigned long touchStartTime = 0;
  static int16_t startTouchY = 0;
  
  if (touchCount > 0) {
    touchX = x[0];
    touchY = y[0];
    
    if (touchY >= 25 && touchY <= 210 && touchX <= 310) {
      if (!touchPressed) {
        touchPressed = true;
        wasTouchPressed = false;
        touchStartTime = millis();
        startTouchY = touchY;
      }
    }
  } else if (touchPressed) {
    touchPressed = false;
    
    unsigned long touchDuration = millis() - touchStartTime;
    int16_t touchMovement = abs(touchY - startTouchY);
    
    if (touchDuration < 500 && touchMovement < 20) {
      handleTouchTap(touchX, startTouchY);
    }
    
    wasTouchPressed = true;
  }
  
  static int16_t lastTouchY = 0;
  static unsigned long lastTouchTime = 0;
  static bool scrolling = false;
  
  if (touchPressed && touchY >= 25 && touchY <= 210 && touchX <= 310) {
    unsigned long currentTime = millis();
    
    if (scrolling && (currentTime - lastTouchTime) > 50) {
      int16_t deltaY = touchY - lastTouchY;
      
      if (deltaY > 10) {
        scrollUp();
        lastTouchTime = currentTime;
      } else if (deltaY < -10) {
        scrollDown();
        lastTouchTime = currentTime;
      }
    }
    
    lastTouchY = touchY;
    scrolling = true;
  } else {
    scrolling = false;
  }
}

void handleTouchTap(int16_t x, int16_t y) {
  if (inHostSelection) {
    int headerLines = 3;
    int startY = 30 + (headerLines * 16);
    
    if (y >= startY && y <= 200) {
      int lineIndex = (y - startY) / 16;
      
      if (lineIndex >= 0 && lineIndex < knownHosts.size()) {
        display.fillRect(0, startY + (lineIndex * 16), 320, 16, TERMINAL_BLUE);
        delay(100);
        
        selectedHostIndex = lineIndex;
        displayHostSelection();
        
        static unsigned long lastTapTime = 0;
        static int lastTappedHost = -1;
        unsigned long currentTime = millis();
        
        if ((currentTime - lastTapTime) < 600 && lastTappedHost == lineIndex) {
          connectToHost(selectedHostIndex);
          inHostSelection = false;
          updateDisplay();
        }
        
        lastTapTime = currentTime;
        lastTappedHost = lineIndex;
      }
    }
  }
  else if (inWifiSetup && !enteringPassword) {
    int headerLines = 3;
    int startY = 30 + (headerLines * 16);
    
    if (y >= startY && y <= 200) {
      int lineIndex = (y - startY) / 16;
      int networkIndex = lineIndex;
      
      if (networkIndex >= 0 && networkIndex < availableNetworks.size()) {
        display.fillRect(0, startY + (networkIndex * 16), 320, 16, TERMINAL_BLUE);
        delay(100);
        
        selectedNetwork = networkIndex;
        displayWiFiSetup();
        
        static unsigned long lastTapTime = 0;
        static int lastTappedNetwork = -1;
        unsigned long currentTime = millis();
        
        if ((currentTime - lastTapTime) < 600 && lastTappedNetwork == networkIndex) {
          currentSSID = availableNetworks[selectedNetwork];
          addOutput(">>> Connecting to: " + currentSSID);
          addOutput("Enter password for this network:");
          enteringPassword = true;
          inputText = "";
          updateDisplay();
        } else {
          addOutput(">>> Selected: " + availableNetworks[networkIndex]);
          addOutput("Double-tap to connect or press ENTER");
          updateDisplay();
        }
        
        lastTapTime = currentTime;
        lastTappedNetwork = networkIndex;
      }
    }
  }
}

void handleTrackball() {
  static unsigned long lastTrackballTime = 0;
  unsigned long currentTime = millis();
  
  if ((currentTime - lastTrackballTime) > 100) {
    if (trackballUpPressed) {
      if (inHostSelection) {
        if (selectedHostIndex > 0) {
          selectedHostIndex--;
          displayHostSelection();
        }
      } else if (inWifiSetup && !enteringPassword) {
        if (selectedNetwork > 0) {
          selectedNetwork--;
          displayWiFiSetup();
        }
      } else {
        scrollUp();
      }
      trackballUpPressed = false;
      lastTrackballTime = currentTime;
    }
    
    if (trackballDownPressed) {
      if (inHostSelection) {
        if (selectedHostIndex < knownHosts.size() - 1) {
          selectedHostIndex++;
          displayHostSelection();
        }
      } else if (inWifiSetup && !enteringPassword) {
        if (selectedNetwork < availableNetworks.size() - 1) {
          selectedNetwork++;
          displayWiFiSetup();
        }
      } else {
        scrollDown();
      }
      trackballDownPressed = false;
      lastTrackballTime = currentTime;
    }
    
    if (trackballLeftPressed) {
      trackballLeftPressed = false;
      lastTrackballTime = currentTime;
    }
    
    if (trackballRightPressed) {
      trackballRightPressed = false;
      lastTrackballTime = currentTime;
    }
    
    if (trackballClickPressed) {
      sendTabCommand();
      trackballClickPressed = false;
      lastTrackballTime = currentTime;
    }
  }
}

void scrollUp() {
  if (scrollOffset > 0) {
    scrollOffset--;
    redrawTerminal();
  }
}

void scrollDown() {
  int maxLines = 12;
  int maxOffset = max(0, (int)terminalBuffer.size() - maxLines);
  
  if (scrollOffset < maxOffset) {
    scrollOffset++;
    redrawTerminal();
  }
}

void sendTabCommand() {
  if (sshConnected && sshClient.connected()) {
    sshClient.write(0x09);
    sshClient.flush();
    
    display.fillRect(290, 225, 15, 10, TERMINAL_YELLOW);
    
    delay(150);
    if (sshClient.available() == 0 && inputText.length() > 0) {
      sshClient.write(0x09);
      sshClient.flush();
    }
    
    delay(50);
    updateDisplay();
  } else {
    if (inputText.length() > 0) {
      String commands[] = {"ssh", "hosts", "scan", "help", "wifi", "saved", "forget", "disconnect", "clear", "exit"};
      int commandCount = 10;
      
      for (int i = 0; i < commandCount; i++) {
        if (commands[i].startsWith(inputText)) {
          inputText = commands[i];
          updateDisplay();
          return;
        }
      }
      
      inputText += "    ";
      updateDisplay();
    } else {
      addOutput("Commands: ssh, hosts, scan, help, wifi, disconnect, clear");
    }
  }
}

void scanWiFiNetworks() {
  availableNetworks.clear();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  addOutput("Scanning for networks...");
  updateDisplay();
  
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    addOutput("No networks found");
  } else {
    addOutput(String(n) + " networks found:");
    for (int i = 0; i < n && i < 10; i++) {
      String network = WiFi.SSID(i);
      if (network.length() > 0) {
        availableNetworks.push_back(network);
      }
    }
    selectedNetwork = 0;
    displayWiFiSetup();
  }
}

void displayWiFiSetup() {
  terminalBuffer.clear();
  addOutput("WiFi Networks (tap or use trackball):");
  addOutput("Tap once to select, double-tap to connect");
  addOutput("");
  
  for (int i = 0; i < availableNetworks.size(); i++) {
    String prefix = (i == selectedNetwork) ? "[*] " : "[ ] ";
    String signalInfo = "";
    
    int32_t rssi = WiFi.RSSI(i);
    if (rssi > -50) signalInfo = " ████";
    else if (rssi > -60) signalInfo = " ███·";
    else if (rssi > -70) signalInfo = " ██··";
    else if (rssi > -80) signalInfo = " █···";
    else signalInfo = " ····";
    
    addOutput(prefix + availableNetworks[i] + signalInfo);
  }
  
  redrawTerminal();
}

void connectToSelectedWiFi() {
  if (currentSSID.length() == 0) return;
  
  addOutput("Connecting to: " + currentSSID);
  updateDisplay();
  
  WiFi.begin(currentSSID.c_str(), wifiPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    addOutput("WiFi connected!");
    addOutput("IP: " + WiFi.localIP().toString());
    addOutput("Type 'hosts' to select SSH host");
    
    // Guardar credenciales WiFi exitosas
    saveWiFiCredentials(currentSSID, wifiPassword);
    addOutput("Network credentials saved");
    
    inWifiSetup = false;
    enteringPassword = false;
  } else {
    addOutput("Connection failed. Try again.");
    enteringPassword = false;
  }
  
  updateDisplay();
}

void disconnectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
    addOutput("WiFi disconnected");
    addOutput("Type 'wifi' to reconnect");
  } else {
    addOutput("WiFi not connected");
  }
}

void updateBatteryInfo() {
  int adcValue = analogRead(BOARD_BAT_ADC);
  batteryVoltage = (adcValue * 3.3 * 2.0) / 4095.0;
  
  if (batteryVoltage >= 4.2) {
    batteryPercent = 100;
  } else if (batteryVoltage <= 3.0) {
    batteryPercent = 0;
  } else {
    batteryPercent = (int)((batteryVoltage - 3.0) / 1.2 * 100);
  }
}

void drawBatteryInfo() {
  display.setFont(&fonts::Font2);
  display.fillRect(220, 2, 100, 18, TERMINAL_BLACK);
  
  uint16_t batteryColor;
  if (batteryPercent > 50) {
    batteryColor = TERMINAL_GREEN;
  } else if (batteryPercent > 20) {
    batteryColor = TERMINAL_YELLOW;
  } else {
    batteryColor = TERMINAL_RED;
  }
  
  display.setTextColor(batteryColor, TERMINAL_BLACK);
  display.setCursor(225, 8);
  display.printf("%d%%", batteryPercent);
  
  int batteryX = 258;
  int batteryY = 4;
  
  display.drawRect(batteryX, batteryY, 18, 10, batteryColor);
  display.drawRect(batteryX + 18, batteryY + 2, 2, 6, batteryColor);
  
  int fillWidth = (batteryPercent * 16) / 100;
  if (fillWidth > 0) {
    display.fillRect(batteryX + 1, batteryY + 1, fillWidth, 8, batteryColor);
  }
}

// ===== FUNCIONES DE PERSISTENCIA =====

void loadSavedNetworks() {
  preferences.begin("wifi", true); // Solo lectura
  
  size_t schLen = preferences.getBytesLength("networks");
  if (schLen == 0) {
    preferences.end();
    Serial.println("No saved WiFi networks found");
    return;
  }
  
  char buffer[schLen];
  preferences.getBytes("networks", buffer, schLen);
  preferences.end();
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, buffer);
  
  if (error) {
    Serial.println("Failed to parse saved networks");
    return;
  }
  
  savedNetworks.clear();
  JsonArray networks = doc.as<JsonArray>();
  
  for (JsonObject network : networks) {
    SavedWiFiNetwork savedNet;
    savedNet.ssid = network["ssid"].as<String>();
    savedNet.password = network["password"].as<String>();
    savedNet.rssi = network["rssi"].as<int>();
    savedNet.lastConnected = network["lastConnected"].as<unsigned long>();
    savedNet.autoConnect = network["autoConnect"].as<bool>();
    
    savedNetworks.push_back(savedNet);
  }
  
  Serial.println("Loaded " + String(savedNetworks.size()) + " saved networks");
}

void saveSavedNetworks() {
  preferences.begin("wifi", false); // Lectura/escritura
  
  DynamicJsonDocument doc(2048);
  JsonArray networks = doc.to<JsonArray>();
  
  for (const SavedWiFiNetwork& network : savedNetworks) {
    JsonObject net = networks.createNestedObject();
    net["ssid"] = network.ssid;
    net["password"] = network.password;
    net["rssi"] = network.rssi;
    net["lastConnected"] = network.lastConnected;
    net["autoConnect"] = network.autoConnect;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  preferences.putBytes("networks", jsonString.c_str(), jsonString.length());
  preferences.end();
  
  Serial.println("Saved " + String(savedNetworks.size()) + " networks to flash");
}

void saveWiFiCredentials(String ssid, String password) {
  // Buscar si la red ya existe
  for (int i = 0; i < savedNetworks.size(); i++) {
    if (savedNetworks[i].ssid == ssid) {
      // Actualizar credenciales existentes
      savedNetworks[i].password = password;
      savedNetworks[i].lastConnected = millis();
      savedNetworks[i].autoConnect = true;
      saveSavedNetworks();
      return;
    }
  }
  
  // Agregar nueva red
  SavedWiFiNetwork newNetwork;
  newNetwork.ssid = ssid;
  newNetwork.password = password;
  newNetwork.rssi = WiFi.RSSI();
  newNetwork.lastConnected = millis();
  newNetwork.autoConnect = true;
  
  savedNetworks.push_back(newNetwork);
  
  // Limitar a 5 redes guardadas máximo
  if (savedNetworks.size() > 5) {
    // Remover la red con conexión más antigua
    int oldestIndex = 0;
    unsigned long oldestTime = savedNetworks[0].lastConnected;
    
    for (int i = 1; i < savedNetworks.size(); i++) {
      if (savedNetworks[i].lastConnected < oldestTime) {
        oldestTime = savedNetworks[i].lastConnected;
        oldestIndex = i;
      }
    }
    
    savedNetworks.erase(savedNetworks.begin() + oldestIndex);
  }
  
  saveSavedNetworks();
}

void loadSSHHosts() {
  preferences.begin("ssh", true); // Solo lectura
  
  size_t schLen = preferences.getBytesLength("hosts");
  if (schLen == 0) {
    preferences.end();
    Serial.println("No saved SSH hosts found");
    return;
  }
  
  char buffer[schLen];
  preferences.getBytes("hosts", buffer, schLen);
  preferences.end();
  
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, buffer);
  
  if (error) {
    Serial.println("Failed to parse saved SSH hosts");
    return;
  }
  
  JsonArray hosts = doc.as<JsonArray>();
  
  // Agregar hosts guardados a la lista (además de los predeterminados)
  for (JsonObject host : hosts) {
    SSHHost savedHost;
    savedHost.name = host["name"].as<String>();
    savedHost.ip = host["ip"].as<String>();
    savedHost.port = host["port"].as<int>();
    savedHost.user = host["user"].as<String>();
    savedHost.isAvailable = false; // Se verificará en el escaneo
    savedHost.lastSeen = host["lastSeen"].as<unsigned long>();
    
    // Verificar que no sea duplicado
    bool isDuplicate = false;
    for (const SSHHost& existing : knownHosts) {
      if (existing.ip == savedHost.ip && existing.port == savedHost.port) {
        isDuplicate = true;
        break;
      }
    }
    
    if (!isDuplicate) {
      knownHosts.push_back(savedHost);
    }
  }
  
  Serial.println("Loaded " + String(hosts.size()) + " saved SSH hosts");
}

void saveSSHHosts() {
  preferences.begin("ssh", false); // Lectura/escritura
  
  DynamicJsonDocument doc(2048);
  JsonArray hosts = doc.to<JsonArray>();
  
  // Solo guardar hosts que no sean los predeterminados
  for (const SSHHost& host : knownHosts) {
    if (host.name.startsWith("SSH-") || host.name == "Custom Host") {
      JsonObject h = hosts.createNestedObject();
      h["name"] = host.name;
      h["ip"] = host.ip;
      h["port"] = host.port;
      h["user"] = host.user;
      h["lastSeen"] = host.lastSeen;
    }
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  preferences.putBytes("hosts", jsonString.c_str(), jsonString.length());
  preferences.end();
  
  Serial.println("Saved SSH hosts to flash");
}

bool attemptAutoConnect() {
  if (autoConnectAttempted || savedNetworks.size() == 0) {
    return false;
  }
  
  autoConnectAttempted = true;
  
  // Buscar la red más reciente con autoConnect habilitado
  SavedWiFiNetwork* bestNetwork = nullptr;
  unsigned long mostRecent = 0;
  
  for (SavedWiFiNetwork& network : savedNetworks) {
    if (network.autoConnect && network.lastConnected > mostRecent) {
      mostRecent = network.lastConnected;
      bestNetwork = &network;
    }
  }
  
  if (bestNetwork == nullptr) {
    return false;
  }
  
  addOutput("Attempting auto-connect to: " + bestNetwork->ssid);
  updateDisplay();
  
  WiFi.begin(bestNetwork->ssid.c_str(), bestNetwork->password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    bestNetwork->lastConnected = millis();
    saveSavedNetworks();
    return true;
  } else {
    // Desactivar autoConnect si falla
    bestNetwork->autoConnect = false;
    saveSavedNetworks();
    addOutput("Auto-connect failed");
    return false;
  }
}