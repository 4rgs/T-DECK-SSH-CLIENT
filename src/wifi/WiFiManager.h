#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <vector>
#include <ArduinoJson.h>

struct WiFiNetwork {
  String ssid;
  int rssi;
  bool isSecure;
};

struct SavedWiFiNetwork {
  String ssid;
  String password;
  int rssi;
  unsigned long lastConnected;
  bool autoConnect;
};

class WiFiManager {
private:
  std::vector<WiFiNetwork> availableNetworks;
  std::vector<SavedWiFiNetwork> savedNetworks;
  int selectedNetworkIndex;
  bool isScanning;
  bool autoConnectAttempted;
  
public:
  WiFiManager();
  
  // Gestión de redes disponibles
  void startScan();
  bool isScanComplete();
  std::vector<WiFiNetwork> getAvailableNetworks();
  void clearNetworks();
  
  // Conexión
  bool connectToNetwork(String ssid, String password);
  bool connectToSavedNetwork(int index);
  void disconnect();
  bool isConnected();
  String getConnectedSSID();
  String getLocalIP();
  int getSignalStrength();
  
  // Navegación de redes
  void selectNextNetwork();
  void selectPrevNetwork();
  int getSelectedNetworkIndex();
  void setSelectedNetworkIndex(int index);
  
  // Persistencia
  void loadSavedNetworks();
  void saveSavedNetworks();
  void saveWiFiCredentials(String ssid, String password);
  std::vector<SavedWiFiNetwork> getSavedNetworks();
  bool forgetNetwork(String ssid);
  
  // Auto-conexión
  bool attemptAutoConnect();
  void resetAutoConnectFlag();
  
  // Estado
  String getStatusString();
  wl_status_t getStatus();
};

extern WiFiManager wifiManager;

#endif