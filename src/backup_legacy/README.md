# 🗄️ Legacy Code Backup

Este directorio contiene las versiones anteriores del código que han sido deprecadas.

## 📁 Contenido

### `main_legacy.cpp`
- **Descripción**: Versión monolítica original del T-Deck SSH Terminal
- **Estado**: ⚠️ **DEPRECATED** - No usar para nuevos desarrollos
- **Fecha de deprecación**: 27 Septiembre 2025
- **Líneas de código**: ~1500+
- **Reemplazado por**: Arquitectura modular en `/src/main.cpp`

### `main.cpp.bak`
- **Descripción**: Backup automático previo
- **Estado**: Archivo de respaldo histórico

## 🚫 Razones de Deprecación

La versión monolítica fue reemplazada por las siguientes razones:

1. **Mantenibilidad**: Código difícil de mantener con 1500+ líneas
2. **Acoplamiento**: Alto acoplamiento entre componentes
3. **Testing**: Difícil realizar pruebas unitarias
4. **Colaboración**: Complicado para trabajo en equipo
5. **Escalabilidad**: Limitada para agregar nuevas funciones
6. **Debugging**: Difícil localizar y corregir errores

## 🏗️ Nueva Arquitectura

El proyecto ahora utiliza una arquitectura modular con:

- **HardwareManager**: Gestión de hardware T-Deck
- **WiFiManager**: Gestión de redes y conectividad  
- **SSHManager**: Conexiones SSH y descubrimiento
- **PersistenceManager**: Almacenamiento persistente
- **UIManager**: Interfaz de usuario
- **CommandProcessor**: Procesamiento de comandos

## 📖 Documentación

Para información completa sobre la nueva arquitectura, ver:
- [`../MODULAR_ARCHITECTURE.md`](../MODULAR_ARCHITECTURE.md)
- [`../README.md`](../README.md)

## ⚡ Uso de Versiones Legacy (No Recomendado)

Si necesitas usar temporalmente la versión legacy:

```bash
# ⚠️ NO RECOMENDADO - Solo para emergencias
cd src
mv main.cpp main_modular.cpp
mv backup_legacy/main_legacy.cpp main.cpp

# Actualizar platformio.ini
# Remover: build_src_filter y usar solo src/main.cpp
```

## 🔄 Migración

Para migrar código personalizado de la versión legacy:

1. Identificar la funcionalidad a migrar
2. Determinar el módulo apropiado en la nueva arquitectura
3. Implementar en el módulo correspondiente
4. Probar la integración

---

**Recomendación**: Use siempre la versión modular para nuevos desarrollos. La versión legacy se mantiene únicamente como referencia histórica.