#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <vector>
#include <Arduino.h>

class CommandProcessor {
private:
  std::vector<String> commandHistory;
  int historyIndex;
  int maxHistorySize;
  
  // Procesadores de comandos específicos
  void processHelpCommand();
  void processWiFiCommand(const std::vector<String>& args);
  void processSSHCommand(const std::vector<String>& args);
  void processSavedCommand();
  void processForgetCommand(const std::vector<String>& args);
  void processHostsCommand();
  void processScanCommand();
  void processClearCommand();
  void processExitCommand();
  void processStatusCommand();
  
  // Utilidades
  std::vector<String> parseCommand(String input);
  String joinArgs(const std::vector<String>& args, int startIndex = 0);
  bool isValidCommand(String command);
  
public:
  CommandProcessor();
  
  // Procesamiento de comandos
  bool executeCommand(String input);
  void addToHistory(String command);
  
  // Navegación del historial
  String getPreviousCommand();
  String getNextCommand();
  void resetHistoryIndex();
  
  // Autocompletado
  std::vector<String> getCommandSuggestions(String partial);
  String getTabCompletion(String partial);
  
  // Gestión del historial
  std::vector<String> getHistory();
  void clearHistory();
  void saveHistory();
  void loadHistory();
  
  // Información de comandos
  std::vector<String> getAvailableCommands();
  String getCommandHelp(String command);
  bool isSSHSession();
};

extern CommandProcessor commandProcessor;

#endif