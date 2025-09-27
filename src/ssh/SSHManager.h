#ifndef SSH_MANAGER_H
#define SSH_MANAGER_H

#include <WiFiClient.h>
#include <vector>
#include <ArduinoJson.h>

struct SSHHost {
  String name;
  String ip;
  int port;
  String user;
  bool isAvailable;
  unsigned long lastSeen;
};

class SSHManager {
private:
  WiFiClient sshClient;
  std::vector<SSHHost> knownHosts;
  int selectedHostIndex;
  bool isConnected;
  bool isScanning;
  String currentHost;
  String currentUser;
  
  // Buffer para datos SSH
  String inputBuffer;
  String outputBuffer;
  
public:
  SSHManager();
  
  // Gestión de conexión SSH
  bool connectToHost(String host, int port, String user, String password);
  bool connectToHost(SSHHost host, String password);
  void disconnect();
  bool isSSHConnected();
  String getCurrentHost();
  String getCurrentUser();
  
  // Envío y recepción de datos
  void sendData(String data);
  String receiveData();
  bool hasIncomingData();
  void processIncomingData();
  
  // Gestión de hosts
  void addKnownHost(SSHHost host);
  void loadKnownHosts();
  void saveKnownHosts();
  std::vector<SSHHost> getKnownHosts();
  void clearKnownHosts();
  
  // Descubrimiento de hosts
  void startNetworkScan();
  bool isScanComplete();
  void scanIPRange(String baseIP, int startRange, int endRange);
  bool checkSSHPort(String ip, int port);
  
  // Navegación de hosts
  void selectNextHost();
  void selectPrevHost();
  int getSelectedHostIndex();
  void setSelectedHostIndex(int index);
  
  // Utilidades
  String parseSSHCommand(String command);
  bool isValidIPAddress(String ip);
  String getNetworkBase();
};

extern SSHManager sshManager;

#endif