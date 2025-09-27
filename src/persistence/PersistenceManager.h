#ifndef PERSISTENCE_MANAGER_H
#define PERSISTENCE_MANAGER_H

#include <Preferences.h>
#include <ArduinoJson.h>
#include <vector>

// Forward declarations
struct SavedWiFiNetwork;
struct SSHHost;

class PersistenceManager {
private:
  Preferences preferences;
  
public:
  PersistenceManager();
  
  // WiFi Networks
  std::vector<SavedWiFiNetwork> loadSavedNetworks();
  void saveSavedNetworks(const std::vector<SavedWiFiNetwork>& networks);
  
  // SSH Hosts
  std::vector<SSHHost> loadSSHHosts();
  void saveSSHHosts(const std::vector<SSHHost>& hosts);
  
  // Configuraciones generales
  void saveConfig(const char* key, const String& value);
  String loadConfig(const char* key, const String& defaultValue = "");
  void saveConfig(const char* key, int value);
  int loadConfig(const char* key, int defaultValue);
  void saveConfig(const char* key, bool value);
  bool loadConfig(const char* key, bool defaultValue);
  
  // Utilidades
  void clearAllData();
  void clearWiFiData();
  void clearSSHData();
  size_t getUsedSpace();
  size_t getTotalSpace();
};

extern PersistenceManager persistenceManager;

#endif