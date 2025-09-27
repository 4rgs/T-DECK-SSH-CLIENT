#include "SSHManager.h"
#include "../persistence/PersistenceManager.h"
#include "../wifi/WiFiManager.h"

SSHManager sshManager;

const int SCAN_START_IP = 1;
const int SCAN_END_IP = 50;
const int SSH_TIMEOUT = 5000;

SSHManager::SSHManager() {
  selectedHostIndex = 0;
  isConnected = false;
  isScanning = false;
  
  // Hosts predeterminados comunes
  addKnownHost({"Raspberry Pi", "192.168.1.100", 22, "pi"});
  addKnownHost({"Ubuntu Server", "192.168.1.101", 22, "ubuntu"});
  addKnownHost({"OpenWrt Router", "192.168.1.1", 22, "root"});
  addKnownHost({"Custom Host", "192.168.1.50", 22, "admin"});
}

bool SSHManager::connectToHost(String host, int port, String user, String password) {
  if (sshClient.connected()) {
    sshClient.stop();
  }
  
  if (sshClient.connect(host.c_str(), port)) {
    isConnected = true;
    currentHost = host;
    currentUser = user;
    
    // Esperar banner SSH
    delay(1000);
    
    // Proceso básico de autenticación SSH (simplificado)
    // En una implementación real se necesitaría un cliente SSH completo
    return true;
  }
  
  return false;
}

bool SSHManager::connectToHost(SSHHost host, String password) {
  return connectToHost(host.ip, host.port, host.user, password);
}

void SSHManager::disconnect() {
  if (sshClient.connected()) {
    sshClient.stop();
  }
  isConnected = false;
  currentHost = "";
  currentUser = "";
}

bool SSHManager::isSSHConnected() {
  return isConnected && sshClient.connected();
}

String SSHManager::getCurrentHost() {
  return currentHost;
}

String SSHManager::getCurrentUser() {
  return currentUser;
}

void SSHManager::sendData(String data) {
  if (isSSHConnected()) {
    sshClient.print(data);
  }
}

String SSHManager::receiveData() {
  String data = "";
  if (isSSHConnected()) {
    while (sshClient.available()) {
      data += (char)sshClient.read();
    }
  }
  return data;
}

bool SSHManager::hasIncomingData() {
  return isSSHConnected() && sshClient.available();
}

void SSHManager::processIncomingData() {
  if (hasIncomingData()) {
    outputBuffer += receiveData();
  }
}

void SSHManager::addKnownHost(SSHHost host) {
  // Verificar que no sea duplicado
  for (const SSHHost& existing : knownHosts) {
    if (existing.ip == host.ip && existing.port == host.port) {
      return;
    }
  }
  
  knownHosts.push_back(host);
}

void SSHManager::loadKnownHosts() {
  std::vector<SSHHost> savedHosts = persistenceManager.loadSSHHosts();
  
  // Agregar hosts guardados (sin duplicar los predeterminados)
  for (const SSHHost& host : savedHosts) {
    addKnownHost(host);
  }
}

void SSHManager::saveKnownHosts() {
  // Solo guardar hosts que no sean los predeterminados
  std::vector<SSHHost> hostsToSave;
  
  for (const SSHHost& host : knownHosts) {
    if (host.name.startsWith("SSH-") || host.name == "Custom Host") {
      hostsToSave.push_back(host);
    }
  }
  
  persistenceManager.saveSSHHosts(hostsToSave);
}

std::vector<SSHHost> SSHManager::getKnownHosts() {
  return knownHosts;
}

void SSHManager::clearKnownHosts() {
  knownHosts.clear();
  selectedHostIndex = 0;
}

void SSHManager::startNetworkScan() {
  if (!wifiManager.isConnected()) {
    return;
  }
  
  isScanning = true;
  String baseIP = getNetworkBase();
  scanIPRange(baseIP, SCAN_START_IP, SCAN_END_IP);
  isScanning = false;
}

bool SSHManager::isScanComplete() {
  return !isScanning;
}

void SSHManager::scanIPRange(String baseIP, int startRange, int endRange) {
  for (int i = startRange; i <= endRange; i++) {
    String ip = baseIP + String(i);
    
    // Verificar puerto 22 (SSH estándar)
    if (checkSSHPort(ip, 22)) {
      SSHHost newHost;
      newHost.name = "SSH-" + ip;
      newHost.ip = ip;
      newHost.port = 22;
      newHost.user = "root";
      newHost.isAvailable = true;
      newHost.lastSeen = millis();
      
      addKnownHost(newHost);
    }
    
    // Verificar puerto 2222 (SSH alternativo)
    if (checkSSHPort(ip, 2222)) {
      SSHHost newHost;
      newHost.name = "SSH-" + ip + ":2222";
      newHost.ip = ip;
      newHost.port = 2222;
      newHost.user = "admin";
      newHost.isAvailable = true;
      newHost.lastSeen = millis();
      
      addKnownHost(newHost);
    }
    
    delay(50); // Pequeña pausa para no sobrecargar la red
  }
  
  saveKnownHosts();
}

bool SSHManager::checkSSHPort(String ip, int port) {
  WiFiClient testClient;
  
  if (testClient.connect(ip.c_str(), port)) {
    testClient.setTimeout(2000);
    
    // Esperar banner SSH
    unsigned long startTime = millis();
    String banner = "";
    
    while (millis() - startTime < 2000) {
      if (testClient.available()) {
        banner += (char)testClient.read();
        if (banner.indexOf("SSH") != -1) {
          testClient.stop();
          return true;
        }
      }
      delay(10);
    }
    
    testClient.stop();
  }
  
  return false;
}

void SSHManager::selectNextHost() {
  if (knownHosts.size() > 0) {
    selectedHostIndex = (selectedHostIndex + 1) % knownHosts.size();
  }
}

void SSHManager::selectPrevHost() {
  if (knownHosts.size() > 0) {
    selectedHostIndex = (selectedHostIndex - 1 + knownHosts.size()) % knownHosts.size();
  }
}

int SSHManager::getSelectedHostIndex() {
  return selectedHostIndex;
}

void SSHManager::setSelectedHostIndex(int index) {
  if (index >= 0 && index < knownHosts.size()) {
    selectedHostIndex = index;
  }
}

String SSHManager::parseSSHCommand(String command) {
  // Parsear comando ssh user@host
  int atIndex = command.indexOf('@');
  if (atIndex > 0) {
    String user = command.substring(0, atIndex);
    String host = command.substring(atIndex + 1);
    
    // Verificar si incluye puerto
    int colonIndex = host.indexOf(':');
    if (colonIndex > 0) {
      String hostIP = host.substring(0, colonIndex);
      int port = host.substring(colonIndex + 1).toInt();
      return user + "@" + hostIP + ":" + String(port);
    }
    
    return user + "@" + host + ":22";
  }
  
  return command;
}

bool SSHManager::isValidIPAddress(String ip) {
  int dots = 0;
  int numbers = 0;
  
  for (int i = 0; i < ip.length(); i++) {
    char c = ip.charAt(i);
    if (c == '.') {
      dots++;
      if (numbers == 0) return false;
      numbers = 0;
    } else if (c >= '0' && c <= '9') {
      numbers++;
    } else {
      return false;
    }
  }
  
  return dots == 3 && numbers > 0;
}

String SSHManager::getNetworkBase() {
  String ip = wifiManager.getLocalIP();
  int lastDot = ip.lastIndexOf('.');
  
  if (lastDot > 0) {
    return ip.substring(0, lastDot + 1);
  }
  
  return "192.168.1.";
}