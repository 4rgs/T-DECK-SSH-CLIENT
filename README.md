# T-Deck SSH Terminal

Un terminal SSH completo para LilyGO T-Deck ESP32-S3 con interfaz táctil, gestión de WiFi y descubrimiento automático de hosts.

## 🏗️ **Nueva Arquitectura Modular v2.0**

El proyecto ha sido **completamente refactorizado** con una arquitectura modular que mejora significativamente la mantenibilidad y escalabilidad:

- 📁 **6 módulos especializados** (Hardware, WiFi, SSH, Persistence, UI, Commands)
- 🔧 **Separación de responsabilidades** para mejor organización
- 🚀 **Fácil mantenimiento y extensión** de funcionalidades
- 📖 **[Ver documentación completa de la arquitectura](MODULAR_ARCHITECTURE.md)**

## 🚀 Características

### 🖥️ Terminal SSH Completo
- Conexión SSH con autenticación por usuario/contraseña
- Soporte completo para secuencias ANSI (colores, cursor, etc.)
- Historial de comandos navegable
- Interfaz táctil y navegación con trackball

### 📡 Gestión Avanzada de WiFi
- Escaneo automático de redes disponibles
- **Persistencia automática** de credenciales WiFi
- **Auto-conexión** inteligente al iniciar
- Comandos para gestionar redes guardadas

### 🔍 Descubrimiento de Hosts SSH
- **Escaneo automático de red** para detectar hosts SSH
- Verificación de puertos 22 y 2222
- **Almacenamiento persistente** de hosts conocidos
- Interfaz de selección táctil para hosts disponibles

### 🔋 Monitoreo del Sistema
- Indicador de batería en tiempo real
- Información de red (IP, señal WiFi)
- Gestión de estado de conexión

## 🛠️ Hardware Requerido

- **LilyGO T-Deck ESP32-S3**
  - Pantalla ST7789 240x320
  - Panel táctil GT911
  - Trackball integrado
  - Teclado físico
  - Batería LiPo con monitoreo

## 📦 Instalación

### Prerrequisitos
```bash
# Instalar PlatformIO CLI
pip install platformio

# O usar PlatformIO IDE extension en VS Code
```

### Compilación y Flash
```bash
# Clonar repositorio
git clone https://github.com/TU_USUARIO/tdeck-ssh-terminal.git
cd tdeck-ssh-terminal

# Compilar
platformio run

# Subir al dispositivo
platformio run --target upload

# Monitor serial (opcional)
platformio device monitor
```

## 🎮 Uso

### Comandos Disponibles

| Comando | Descripción |
|---------|-------------|
| `help` | Muestra ayuda completa |
| `wifi scan` | Escanea redes WiFi |
| `wifi connect <ssid>` | Conecta a red WiFi |
| `wifi disconnect` | Desconecta WiFi |
| `wifi status` | Estado de conexión |
| `saved` | **Lista redes WiFi guardadas** |
| `forget <ssid>` | **Olvida red WiFi guardada** |
| `ssh <user@host>` | Conecta por SSH |
| `hosts` | **Muestra hosts SSH conocidos** |
| `scan` | **Escanea red para hosts SSH** |
| `clear` | Limpia pantalla |
| `exit` | Sale de sesión SSH |

### Navegación
- **Táctil**: Toca directamente en la pantalla
- **Trackball**: Navegación precisa del cursor
- **Teclado físico**: Entrada de texto completa
- **Selección de hosts**: Interfaz táctil para elegir destinos SSH

## 🔧 Configuración

### Personalización de Red
```cpp
// En src/main.cpp, personalizar rango de escaneo
const int SCAN_START_IP = 1;    // IP inicial
const int SCAN_END_IP = 50;     // IP final
```

### Hosts SSH Predeterminados
```cpp
// Agregar hosts conocidos en setup()
knownHosts.push_back({"Mi Servidor", "192.168.1.100", 22, "admin"});
```

## 🗂️ Estructura del Proyecto

```
tdeck_ssh_terminal/
├── src/
│   ├── main.cpp                 # 🚀 Código principal (arquitectura modular v2.0)
│   ├── hardware/                # 🔧 Gestión de hardware T-Deck
│   ├── wifi/                    # 📡 Gestión WiFi y redes
│   ├── ssh/                     # 🖥️ Conexiones SSH y hosts
│   ├── persistence/             # 💾 Almacenamiento persistente
│   ├── ui/                      # 🎨 Interfaz de usuario
│   ├── commands/                # ⌨️ Procesamiento de comandos
│   ├── keyboard/                # ⌨️ Entrada de teclado
│   ├── display/                 # 🖼️ Configuración de pantalla
│   └── backup_legacy/           # 🗄️ Código legacy (deprecated)
├── include/
│   └── config.h                # ⚙️ Configuración centralizada
├── platformio.ini              # 🔧 Configuración PlatformIO
├── README.md                   # 📖 Este archivo
└── MODULAR_ARCHITECTURE.md     # 🏗️ Documentación de la arquitectura
```

## 🔄 Sistema de Persistencia

### Almacenamiento Automático
- **Redes WiFi**: Se guardan automáticamente al conectarse
- **Hosts SSH**: Discovered hosts se almacenan persistentemente
- **Preferencias**: Uso de ESP32 Preferences (NVRAM)

### Gestión de Datos
- Máximo 5 redes WiFi guardadas
- Auto-eliminación de redes más antiguas
- Timestamps para conexiones recientes
- Serialización JSON para datos complejos

## 🧩 Dependencias

- `bblanchon/ArduinoJson@^7.0.4` - Serialización de datos
- `lovyan03/LovyanGFX@^1.1.16` - Gráficos y pantalla
- ESP32 Preferences - Almacenamiento persistente

## 📋 Características Técnicas

### Rendimiento
- **RAM**: ~14.5% utilizada (47KB/327KB)
- **Flash**: ~25% utilizada (836KB/3.3MB)
- **Interfaz**: 60fps en operaciones gráficas
- **Red**: Auto-reconexión en caso de pérdida de señal

### Seguridad
- Almacenamiento seguro de credenciales en NVRAM
- No exposición de contraseñas en logs
- Gestión de timeouts para conexiones SSH

## 🚧 Próximas Características

- [ ] Soporte para claves SSH (autenticación por llave)
- [ ] Múltiples sesiones SSH simultáneas
- [ ] Configuración de túneles SSH
- [ ] Backup/restore de configuraciones
- [ ] Themes personalizables para la interfaz

## 🤝 Contribuciones

¡Las contribuciones son bienvenidas! Por favor:

1. Fork el proyecto
2. Crea una rama para tu feature (`git checkout -b feature/AmazingFeature`)
3. Commit tus cambios (`git commit -m 'Add some AmazingFeature'`)
4. Push a la rama (`git push origin feature/AmazingFeature`)
5. Abre un Pull Request

## 📄 Licencia

Este proyecto está bajo la Licencia MIT - ver el archivo [LICENSE](LICENSE) para más detalles.

## 🙏 Agradecimientos

- [LilyGO](https://github.com/Xinyuan-LilyGO) por el hardware T-Deck
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) por la biblioteca gráfica
- Comunidad de PlatformIO por las herramientas de desarrollo

---

**Desarrollado con ❤️ para la comunidad maker y entusiastas del IoT**