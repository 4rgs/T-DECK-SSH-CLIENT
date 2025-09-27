#include "PersistenceManager.h"
#include "../wifi/WiFiManager.h"
#include "../ssh/SSHManager.h"

PersistenceManager persistenceManager;

PersistenceManager::PersistenceManager() {
  // Constructor vacío - preferences se inicializa cuando se necesita
}

std::vector<SavedWiFiNetwork> PersistenceManager::loadSavedNetworks() {
  std::vector<SavedWiFiNetwork> networks;
  
  preferences.begin("wifi", true); // Solo lectura
  
  size_t schLen = preferences.getBytesLength("networks");
  if (schLen == 0) {
    preferences.end();
    Serial.println("No saved WiFi networks found");
    return networks;
  }
  
  char buffer[schLen];
  preferences.getBytes("networks", buffer, schLen);
  preferences.end();
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, buffer);
  
  if (error) {
    Serial.println("Failed to parse saved networks");
    return networks;
  }
  
  JsonArray networksArray = doc.as<JsonArray>();
  
  for (JsonObject network : networksArray) {
    SavedWiFiNetwork savedNet;
    savedNet.ssid = network["ssid"].as<String>();
    savedNet.password = network["password"].as<String>();
    savedNet.rssi = network["rssi"].as<int>();
    savedNet.lastConnected = network["lastConnected"].as<unsigned long>();
    savedNet.autoConnect = network["autoConnect"].as<bool>();
    
    networks.push_back(savedNet);
  }
  
  Serial.println("Loaded " + String(networks.size()) + " saved networks");
  return networks;
}

void PersistenceManager::saveSavedNetworks(const std::vector<SavedWiFiNetwork>& networks) {
  preferences.begin("wifi", false); // Lectura/escritura
  
  JsonDocument doc;
  JsonArray networksArray = doc.to<JsonArray>();
  
  for (const SavedWiFiNetwork& network : networks) {
    JsonObject net = networksArray.add<JsonObject>();
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
  
  Serial.println("Saved " + String(networks.size()) + " networks to flash");
}

std::vector<SSHHost> PersistenceManager::loadSSHHosts() {
  std::vector<SSHHost> hosts;
  
  preferences.begin("ssh", true); // Solo lectura
  
  size_t schLen = preferences.getBytesLength("hosts");
  if (schLen == 0) {
    preferences.end();
    Serial.println("No saved SSH hosts found");
    return hosts;
  }
  
  char buffer[schLen];
  preferences.getBytes("hosts", buffer, schLen);
  preferences.end();
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, buffer);
  
  if (error) {
    Serial.println("Failed to parse saved SSH hosts");
    return hosts;
  }
  
  JsonArray hostsArray = doc.as<JsonArray>();
  
  for (JsonObject host : hostsArray) {
    SSHHost savedHost;
    savedHost.name = host["name"].as<String>();
    savedHost.ip = host["ip"].as<String>();
    savedHost.port = host["port"].as<int>();
    savedHost.user = host["user"].as<String>();
    savedHost.isAvailable = false; // Se verificará en el escaneo
    savedHost.lastSeen = host["lastSeen"].as<unsigned long>();
    
    hosts.push_back(savedHost);
  }
  
  Serial.println("Loaded " + String(hosts.size()) + " saved SSH hosts");
  return hosts;
}

void PersistenceManager::saveSSHHosts(const std::vector<SSHHost>& hosts) {
  preferences.begin("ssh", false); // Lectura/escritura
  
  JsonDocument doc;
  JsonArray hostsArray = doc.to<JsonArray>();
  
  for (const SSHHost& host : hosts) {
    JsonObject h = hostsArray.add<JsonObject>();
    h["name"] = host.name;
    h["ip"] = host.ip;
    h["port"] = host.port;
    h["user"] = host.user;
    h["lastSeen"] = host.lastSeen;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  preferences.putBytes("hosts", jsonString.c_str(), jsonString.length());
  preferences.end();
  
  Serial.println("Saved SSH hosts to flash");
}

void PersistenceManager::saveConfig(const char* key, const String& value) {
  preferences.begin("config", false);
  preferences.putString(key, value);
  preferences.end();
}

String PersistenceManager::loadConfig(const char* key, const String& defaultValue) {
  preferences.begin("config", true);
  String value = preferences.getString(key, defaultValue);
  preferences.end();
  return value;
}

void PersistenceManager::saveConfig(const char* key, int value) {
  preferences.begin("config", false);
  preferences.putInt(key, value);
  preferences.end();
}

int PersistenceManager::loadConfig(const char* key, int defaultValue) {
  preferences.begin("config", true);
  int value = preferences.getInt(key, defaultValue);
  preferences.end();
  return value;
}

void PersistenceManager::saveConfig(const char* key, bool value) {
  preferences.begin("config", false);
  preferences.putBool(key, value);
  preferences.end();
}

bool PersistenceManager::loadConfig(const char* key, bool defaultValue) {
  preferences.begin("config", true);
  bool value = preferences.getBool(key, defaultValue);
  preferences.end();
  return value;
}

void PersistenceManager::clearAllData() {
  clearWiFiData();
  clearSSHData();
  
  preferences.begin("config", false);
  preferences.clear();
  preferences.end();
}

void PersistenceManager::clearWiFiData() {
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
}

void PersistenceManager::clearSSHData() {
  preferences.begin("ssh", false);
  preferences.clear();
  preferences.end();
}

size_t PersistenceManager::getUsedSpace() {
  size_t total = 0;
  
  preferences.begin("wifi", true);
  total += preferences.getBytesLength("networks");
  preferences.end();
  
  preferences.begin("ssh", true);
  total += preferences.getBytesLength("hosts");
  preferences.end();
  
  return total;
}

size_t PersistenceManager::getTotalSpace() {
  // ESP32 Preferences usa una partición NVS, típicamente 64KB
  return 65536; // 64KB
}