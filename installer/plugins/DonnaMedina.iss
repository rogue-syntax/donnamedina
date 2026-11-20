; Donna Medina. Build: iscc plugins\DonnaMedina.iss /DPluginVersion=1.0.1  ->  output\DonnaMedinaSetup.exe
; (or installer\build.bat, which copies the Release build into in\ and takes the version from source\version.h)
; Identity from the name "Donna Medina" (store spec licensing-v1.md 1.7): product donnamedina, registry
; subkey DonnaMedina, licence prefix DM, shop page https://rogue-syntax.com/donna-medina.
#define PluginName "Donna Medina"
#define PluginKey "DonnaMedina"
; Installer ID: UUID v5 (DNS namespace) of "donnamedina.plugins.rogue-syntax.com"; never changes once shipped
#define AppGuid "5B54E8DA-C62E-574D-B7D1-A0F7AD12F38E"
#ifndef PluginVersion
  #define PluginVersion "1.0.1"
#endif
#ifndef Vst3Source
  #define Vst3Source "..\in\DonnaMedinaVST64.vst3"
#endif
#include "..\rogue-syntax.iss"
