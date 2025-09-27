#include "WiFiManager.h"
#include "../persistence/PersistenceManager.h"

WiFiManager wifiManager;

WiFiManager::WiFiManager() {
  selectedNetworkIndex = 0;
  isScanning = false;
  autoConnectAttempted = false;
}

void WiFiManager::startScan() {
  if (!isScanning) {
    isScanning = true;
    WiFi.scanDelete();
    WiFi.scanNetworks(true); // Async scan
  }
}

bool WiFiManager::isScanComplete() {
  if (!isScanning) return false;
  
  int scanResult = WiFi.scanComplete();
  if (scanResult >= 0) {
    availableNetworks.clear();
    
    for (int i = 0; i < scanResult; i++) {
      WiFiNetwork network;
      network.ssid = WiFi.SSID(i);
      network.rssi = WiFi.RSSI(i);
      network.isSecure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      
      // Evitar duplicados
      bool isDuplicate = false;
      for (const WiFiNetwork& existing : availableNetworks) {
        if (existing.ssid == network.ssid) {
          isDuplicate = true;
          break;
        }
      }
      
      if (!isDuplicate && network.ssid.length() > 0) {
        availableNetworks.push_back(network);
      }
    }
    
    isScanning = false;
    selectedNetworkIndex = 0;
    return true;
  }
  
  return false;
}

std::vector<WiFiNetwork> WiFiManager::getAvailableNetworks() {
  return availableNetworks;
}

void WiFiManager::clearNetworks() {
  availableNetworks.clear();
  selectedNetworkIndex = 0;
}

bool WiFiManager::connectToNetwork(String ssid, String password) {
  WiFi.begin(ssid.c_str(), password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    saveWiFiCredentials(ssid, password);
    return true;
  }
  
  return false;
}

bool WiFiManager::connectToSavedNetwork(int index) {
  if (index >= 0 && index < savedNetworks.size()) {
    return connectToNetwork(savedNetworks[index].ssid, savedNetworks[index].password);
  }
  return false;
}

void WiFiManager::disconnect() {
  WiFi.disconnect();
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getConnectedSSID() {
  if (isConnected()) {
    return WiFi.SSID();
  }
  return "";
}

String WiFiManager::getLocalIP() {
  if (isConnected()) {
    return WiFi.localIP().toString();
  }
  return "0.0.0.0";
}

int WiFiManager::getSignalStrength() {
  if (isConnected()) {
    return WiFi.RSSI();
  }
  return -100;
}

void WiFiManager::selectNextNetwork() {
  if (availableNetworks.size() > 0) {
    selectedNetworkIndex = (selectedNetworkIndex + 1) % availableNetworks.size();
  }
}

void WiFiManager::selectPrevNetwork() {
  if (availableNetworks.size() > 0) {
    selectedNetworkIndex = (selectedNetworkIndex - 1 + availableNetworks.size()) % availableNetworks.size();
  }
}

int WiFiManager::getSelectedNetworkIndex() {
  return selectedNetworkIndex;
}

void WiFiManager::setSelectedNetworkIndex(int index) {
  if (index >= 0 && index < availableNetworks.size()) {
    selectedNetworkIndex = index;
  }
}

void WiFiManager::loadSavedNetworks() {
  savedNetworks = persistenceManager.loadSavedNetworks();
}

void WiFiManager::saveSavedNetworks() {
  persistenceManager.saveSavedNetworks(savedNetworks);
}

void WiFiManager::saveWiFiCredentials(String ssid, String password) {
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

std::vector<SavedWiFiNetwork> WiFiManager::getSavedNetworks() {
  return savedNetworks;
}

bool WiFiManager::forgetNetwork(String ssid) {
  for (int i = 0; i < savedNetworks.size(); i++) {
    if (savedNetworks[i].ssid == ssid) {
      savedNetworks.erase(savedNetworks.begin() + i);
      saveSavedNetworks();
      return true;
    }
  }
  return false;
}

bool WiFiManager::attemptAutoConnect() {
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
    return false;
  }
}

void WiFiManager::resetAutoConnectFlag() {
  autoConnectAttempted = false;
}

String WiFiManager::getStatusString() {
  switch (WiFi.status()) {
    case WL_CONNECTED:
      return "Connected";
    case WL_DISCONNECTED:
      return "Disconnected";
    case WL_NO_SHIELD:
      return "No WiFi";
    case WL_IDLE_STATUS:
      return "Idle";
    case WL_SCAN_COMPLETED:
      return "Scan Complete";
    case WL_CONNECT_FAILED:
      return "Connect Failed";
    case WL_CONNECTION_LOST:
      return "Connection Lost";
    default:
      return "Connecting";
  }
}

wl_status_t WiFiManager::getStatus() {
  return WiFi.status();
}