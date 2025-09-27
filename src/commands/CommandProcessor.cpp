#include "CommandProcessor.h"
#include "../../include/config.h"
#include "../wifi/WiFiManager.h"
#include "../ssh/SSHManager.h"
#include "../ui/UIManager.h"
#include "../persistence/PersistenceManager.h"

CommandProcessor commandProcessor;

CommandProcessor::CommandProcessor() {
  historyIndex = 0;
  maxHistorySize = MAX_HISTORY_SIZE;
}

bool CommandProcessor::executeCommand(String input) {
  std::vector<String> args = parseCommand(input);
  
  if (args.size() == 0) {
    return false;
  }
  
  String command = args[0];
  command.toLowerCase();
  
  // Add to history
  addToHistory(input);
  
  // Process commands
  if (command == "help") {
    processHelpCommand();
    return true;
  }
  else if (command == "wifi") {
    processWiFiCommand(args);
    return true;
  }
  else if (command == "ssh") {
    processSSHCommand(args);
    return true;
  }
  else if (command == "saved") {
    processSavedCommand();
    return true;
  }
  else if (command == "forget") {
    processForgetCommand(args);
    return true;
  }
  else if (command == "hosts") {
    processHostsCommand();
    return true;
  }
  else if (command == "password" || command == "pass") {
    processPasswordCommand();
    return true;
    return true;
  }
  else if (command == "scan") {
    processScanCommand();
    return true;
  }
  else if (command == "clear") {
    processClearCommand();
    return true;
  }
  else if (command == "exit") {
    processExitCommand();
    return true;
  }
  else if (command == "status") {
    processStatusCommand();
    return true;
  }
  
  return false; // Command not recognized
}

void CommandProcessor::processHelpCommand() {
  uiManager.showHelp();
}

void CommandProcessor::processWiFiCommand(const std::vector<String>& args) {
  if (args.size() < 2) {
    uiManager.addOutput("Usage: wifi <scan|connect|disconnect|status>");
    return;
  }
  
  String subcommand = args[1];
  subcommand.toLowerCase();
  
  if (subcommand == "scan") {
    uiManager.addOutput("Scanning for WiFi networks...");
    wifiManager.startScan();
    
    // Wait for scan to complete (simple blocking approach)
    while (!wifiManager.isScanComplete()) {
      delay(100);
    }
    
    std::vector<WiFiNetwork> networks = wifiManager.getAvailableNetworks();
    uiManager.showWiFiNetworks(networks, wifiManager.getSelectedNetworkIndex());
  }
  else if (subcommand == "connect") {
    if (args.size() < 3) {
      uiManager.addOutput("Usage: wifi connect <ssid>");
      return;
    }
    
    String ssid = joinArgs(args, 2);
    uiManager.addOutput("Enter password for " + ssid + ":");
    // Password input would be handled in main loop
  }
  else if (subcommand == "disconnect") {
    wifiManager.disconnect();
    uiManager.addOutput("WiFi disconnected");
  }
  else if (subcommand == "status") {
    if (wifiManager.isConnected()) {
      uiManager.addOutput("WiFi Status: Connected");
      uiManager.addOutput("SSID: " + wifiManager.getConnectedSSID());
      uiManager.addOutput("IP: " + wifiManager.getLocalIP());
      uiManager.addOutput("Signal: " + String(wifiManager.getSignalStrength()) + " dBm");
    } else {
      uiManager.addOutput("WiFi Status: Disconnected");
    }
  }
  else {
    uiManager.addOutput("Unknown WiFi command: " + subcommand);
  }
}

void CommandProcessor::processSSHCommand(const std::vector<String>& args) {
  if (args.size() < 2) {
    uiManager.addOutput("Usage: ssh <user@host[:port]> [password]");
    uiManager.addOutput("Examples:");
    uiManager.addOutput("  ssh user@192.168.1.100");
    uiManager.addOutput("  ssh user@192.168.1.100:2324");  
    uiManager.addOutput("  ssh user@192.168.1.100 mypassword");
    uiManager.addOutput("  ssh user@192.168.1.100:2324 mypassword");
    return;
  }
  
  String target = args[1];
  String password = "";
  
  // Verificar si se proporcionó contraseña como parámetro
  if (args.size() >= 3) {
    password = joinArgs(args, 2);
  }
  
  // Parsear user@host[:port]
  int atIndex = target.indexOf('@');
  if (atIndex == -1) {
    uiManager.addOutput("Invalid format. Use: user@host[:port]");
    return;
  }
  
  String user = target.substring(0, atIndex);
  String hostPart = target.substring(atIndex + 1);
  
  String host;
  int port = 22; // Puerto SSH por defecto
  
  int colonIndex = hostPart.indexOf(':');
  if (colonIndex != -1) {
    host = hostPart.substring(0, colonIndex);
    port = hostPart.substring(colonIndex + 1).toInt();
    if (port <= 0) port = 22;
  } else {
    host = hostPart;
  }
  
  uiManager.addOutput("SSH connection to: " + user + "@" + host + ":" + String(port));
  
  if (password.length() > 0) {
    // Conectar con contraseña proporcionada
    uiManager.addOutput("Connecting with provided password...");
    
    if (sshManager.connectToHost(host, port, user, password)) {
      uiManager.addOutput("SSH connection established!");
    } else {
      uiManager.addOutput("SSH connection failed!");
    }
  } else {
    // Iniciar conexión sin contraseña - pedirá password interactivamente
    uiManager.addOutput("Connecting... (password will be requested)");
    
    extern bool isInSSHPasswordMode;
    extern String currentInput;
    extern int cursorX, cursorY;
    
    // Iniciar conexión SSH sin password (fallará pero permitirá detectar el prompt)
    if (sshManager.connectToHost(host, port, user, "")) {
      // Conexión exitosa sin password (unlikely pero posible con keys)
      uiManager.addOutput("SSH connection established!");
    } else {
      // Activar modo password manualmente
      isInSSHPasswordMode = true;
      currentInput = "";
      cursorX = 0;
      cursorY = 0;
      
      uiManager.addOutput("Enter password:");
      uiManager.setInput("");
      uiManager.updateDisplay();
    }
  }
}

void CommandProcessor::processSavedCommand() {
  std::vector<SavedWiFiNetwork> networks = wifiManager.getSavedNetworks();
  uiManager.showSavedNetworks(networks);
}

void CommandProcessor::processForgetCommand(const std::vector<String>& args) {
  if (args.size() < 2) {
    uiManager.addOutput("Usage: forget <ssid>");
    return;
  }
  
  String ssid = joinArgs(args, 1);
  if (wifiManager.forgetNetwork(ssid)) {
    uiManager.addOutput("Forgot network: " + ssid);
  } else {
    uiManager.addOutput("Network not found: " + ssid);
  }
}

void CommandProcessor::processHostsCommand() {
  std::vector<SSHHost> hosts = sshManager.getKnownHosts();
  uiManager.showSSHHosts(hosts, sshManager.getSelectedHostIndex());
}

void CommandProcessor::processScanCommand() {
  uiManager.addOutput("Scanning network for SSH hosts...");
  sshManager.startNetworkScan();
  
  // Simple blocking approach - in real implementation this would be async
  while (!sshManager.isScanComplete()) {
    delay(100);
  }
  
  uiManager.addOutput("Network scan complete");
  std::vector<SSHHost> hosts = sshManager.getKnownHosts();
  uiManager.showSSHHosts(hosts, sshManager.getSelectedHostIndex());
}

void CommandProcessor::processClearCommand() {
  uiManager.clearOutput();
}

void CommandProcessor::processExitCommand() {
  if (sshManager.isSSHConnected()) {
    sshManager.disconnect();
    uiManager.addOutput("SSH session terminated");
  } else {
    uiManager.addOutput("No active SSH session");
  }
}

void CommandProcessor::processStatusCommand() {
  uiManager.showStatus();
}

std::vector<String> CommandProcessor::parseCommand(String input) {
  std::vector<String> args;
  input.trim();
  
  if (input.length() == 0) {
    return args;
  }
  
  int start = 0;
  int pos = 0;
  
  while (pos < input.length()) {
    if (input.charAt(pos) == ' ') {
      if (pos > start) {
        args.push_back(input.substring(start, pos));
      }
      
      // Skip multiple spaces
      while (pos < input.length() && input.charAt(pos) == ' ') {
        pos++;
      }
      start = pos;
    } else {
      pos++;
    }
  }
  
  // Add last argument
  if (pos > start) {
    args.push_back(input.substring(start, pos));
  }
  
  return args;
}

String CommandProcessor::joinArgs(const std::vector<String>& args, int startIndex) {
  String result = "";
  
  for (int i = startIndex; i < args.size(); i++) {
    if (i > startIndex) {
      result += " ";
    }
    result += args[i];
  }
  
  return result;
}

bool CommandProcessor::isValidCommand(String command) {
  std::vector<String> validCommands = getAvailableCommands();
  
  for (const String& cmd : validCommands) {
    if (cmd == command) {
      return true;
    }
  }
  
  return false;
}

void CommandProcessor::addToHistory(String command) {
  if (command.length() == 0) return;
  
  // Don't add duplicates
  if (commandHistory.size() > 0 && commandHistory.back() == command) {
    return;
  }
  
  commandHistory.push_back(command);
  
  // Limit history size
  while (commandHistory.size() > maxHistorySize) {
    commandHistory.erase(commandHistory.begin());
  }
  
  historyIndex = commandHistory.size();
}

String CommandProcessor::getPreviousCommand() {
  if (commandHistory.size() == 0) return "";
  
  if (historyIndex > 0) {
    historyIndex--;
  }
  
  return commandHistory[historyIndex];
}

String CommandProcessor::getNextCommand() {
  if (commandHistory.size() == 0) return "";
  
  if (historyIndex < commandHistory.size() - 1) {
    historyIndex++;
    return commandHistory[historyIndex];
  } else {
    historyIndex = commandHistory.size();
    return "";
  }
}

void CommandProcessor::resetHistoryIndex() {
  historyIndex = commandHistory.size();
}

std::vector<String> CommandProcessor::getCommandSuggestions(String partial) {
  std::vector<String> suggestions;
  std::vector<String> allCommands = getAvailableCommands();
  
  partial.toLowerCase();
  
  for (const String& cmd : allCommands) {
    String lowerCmd = cmd;
    lowerCmd.toLowerCase();
    
    if (lowerCmd.startsWith(partial)) {
      suggestions.push_back(cmd);
    }
  }
  
  return suggestions;
}

String CommandProcessor::getTabCompletion(String partial) {
  std::vector<String> suggestions = getCommandSuggestions(partial);
  
  if (suggestions.size() == 1) {
    return suggestions[0] + " ";
  } else if (suggestions.size() > 1) {
    // Find common prefix
    String common = suggestions[0];
    
    for (int i = 1; i < suggestions.size(); i++) {
      int j = 0;
      while (j < common.length() && j < suggestions[i].length() &&
             common.charAt(j) == suggestions[i].charAt(j)) {
        j++;
      }
      common = common.substring(0, j);
    }
    
    return common;
  }
  
  return partial;
}

std::vector<String> CommandProcessor::getHistory() {
  return commandHistory;
}

void CommandProcessor::clearHistory() {
  commandHistory.clear();
  historyIndex = 0;
}

void CommandProcessor::saveHistory() {
  // Save to persistence manager
  // Implementation would serialize history to NVRAM
}

void CommandProcessor::loadHistory() {
  // Load from persistence manager
  // Implementation would deserialize history from NVRAM
}

std::vector<String> CommandProcessor::getAvailableCommands() {
  std::vector<String> commands;
  
  commands.push_back("help");
  commands.push_back("wifi");
  commands.push_back("ssh");
  commands.push_back("saved");
  commands.push_back("forget");
  commands.push_back("hosts");
  commands.push_back("scan");
  commands.push_back("clear");
  commands.push_back("exit");
  commands.push_back("status");
  
  return commands;
}

String CommandProcessor::getCommandHelp(String command) {
  if (command == "help") {
    return "help - Show available commands";
  } else if (command == "wifi") {
    return "wifi <scan|connect|disconnect|status> - WiFi management";
  } else if (command == "ssh") {
    return "ssh <user@host[:port]> - Connect via SSH";
  } else if (command == "saved") {
    return "saved - List saved WiFi networks";
  } else if (command == "forget") {
    return "forget <ssid> - Remove saved network";
  } else if (command == "hosts") {
    return "hosts - List known SSH hosts";
  } else if (command == "scan") {
    return "scan - Scan network for SSH hosts";
  } else if (command == "clear") {
    return "clear - Clear terminal screen";
  } else if (command == "exit") {
    return "exit - Disconnect SSH session";
  } else if (command == "status") {
    return "status - Show system status";
  } else if (command == "password" || command == "pass") {
    return "password - Manually enter SSH password mode";
  }
  
  return "Unknown command: " + command;
}

void CommandProcessor::processPasswordCommand() {
  extern bool isInSSHPasswordMode;
  extern String currentInput;
  extern int cursorX, cursorY;
  
  if (sshManager.isSSHConnecting()) {
    isInSSHPasswordMode = true;
    currentInput = "";
    cursorX = 0;
    cursorY = 0;
    
    uiManager.addOutput("SSH Password mode activated manually");
    uiManager.addOutput("Enter your password:");
    uiManager.setInput("");
    uiManager.updateDisplay();
  } else {
    uiManager.addOutput("No SSH connection or connection attempt active");
    uiManager.updateDisplay();
  }
}

bool CommandProcessor::isSSHSession() {
  return sshManager.isSSHConnected();
}