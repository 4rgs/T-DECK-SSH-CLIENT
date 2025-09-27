#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>

class Terminal {
public:
  explicit Terminal(LovyanGFX& lcd)
  : _lcd(lcd) {}

  void begin(int width, int height, int marginX, int marginY, int fw, int fh, bool statusBar) {
    _scrW = width; _scrH = height; _mx = marginX; _my = marginY;
    _fw = fw; _fh = fh; _status = statusBar;
    _topY = _my + (_status ? 12 : 0);
    _cols = max(1, (_scrW - 2*_mx) / _fw);
    _rows = max(1, (_scrH - _topY - _my) / _fh);
    _cx = 0; _cy = 0;
    _lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    _lcd.setTextSize(1);
    _lcd.setFont(&fonts::Font0);
  }

  void clear() {
    _lcd.fillRect(0, _topY, _scrW, _scrH - _topY, TFT_BLACK);
    _cx = _cy = 0;
  }

  void feed(const char* data, int len) {
    for (int i = 0; i < len; ++i) {
      char ch = data[i];
      if (_escState) {
        handleEsc(ch);
        continue;
      }
      switch (ch) {
        case '\r': _cx = 0; break;
        case '\n': newline(); break;
        case '\b': if (_cx>0){_cx--; drawChar(' '); _cx--; } break;
        case '\t': tab(); break;
        case 0x1B:  // ESC
          startEsc();
          break;
        default:
          if (ch >= 0x20 && ch <= 0x7E) {
            drawChar(ch);
            advance();
          }
          break;
      }
    }
  }

private:
  LovyanGFX& _lcd;
  int _scrW=0, _scrH=0, _mx=0, _my=0, _fw=6, _fh=8, _cols=0, _rows=0;
  int _cx=0, _cy=0; // cursor in cells
  int _topY=0;      // top pixel after status bar
  bool _status=false;

  // ESC / ANSI minimal parser
  bool _escState=false;
  bool _csi=false;
  String _escBuf;

  void drawChar(char c) {
    int px = _mx + _cx * _fw;
    int py = _topY + _cy * _fh;
    _lcd.setCursor(px, py);
    _lcd.print(c);
  }

  void advance() {
    _cx++;
    if (_cx >= _cols) {
      _cx = 0;
      newline();
    }
  }

  void newline() {
    _cy++;
    if (_cy >= _rows) {
      // scroll up one line
      _lcd.scroll(0, -_fh);
      // clear bottom line
      _lcd.fillRect(_mx, _topY + (_rows-1)*_fh, _scrW - 2*_mx, _fh, TFT_BLACK);
      _cy = _rows - 1;
    }
    _cx = 0;
  }

  void tab() {
    int nextTab = ((_cx / 4) + 1) * 4;
    while (_cx < nextTab) { drawChar(' '); _cx++; }
    if (_cx >= _cols) { _cx = 0; newline(); }
  }

  void startEsc() { _escState = true; _csi = false; _escBuf = ""; }

  void handleEsc(char ch) {
    if (!_csi) {
      if (ch == '[') { _csi = true; return; }
      // Simple single-char ESCs not implemented; exit
      _escState = false; _csi = false; _escBuf = "";
      return;
    }
    // CSI sequence: accumulate until a letter
    if ((ch >= '0' && ch <= '9') || ch == ';') {
      _escBuf += ch;
      return;
    }
    // Final byte
    switch (ch) {
      case 'H': case 'f': { // CUP: row;col (1-based)
        int row=1, col=1; parseTwo(_escBuf, row, col);
        row = max(1, min(_rows, row));
        col = max(1, min(_cols, col));
        _cy = row-1; _cx = col-1; break; }
      case 'A': { // CUU n
        int n = parseOne(_escBuf, 1); _cy = max(0, _cy - n); break; }
      case 'B': { // CUD n
        int n = parseOne(_escBuf, 1); _cy = min(_rows-1, _cy + n); break; }
      case 'C': { // CUF n
        int n = parseOne(_escBuf, 1); _cx = min(_cols-1, _cx + n); break; }
      case 'D': { // CUB n
        int n = parseOne(_escBuf, 1); _cx = max(0, _cx - n); break; }
      case 'K': { // EL
        int px = _mx + _cx * _fw;
        int py = _topY + _cy * _fh;
        _lcd.fillRect(px, py, _scrW - px - _mx, _fh, TFT_BLACK);
        break; }
      case 'J': { // ED: clear screen
        clear();
        break; }
      case 'm': { // SGR colors (very minimal)
        // parse attributes but only handle reset (0)
        int n = parseOne(_escBuf, 0);
        if (n == 0) { _lcd.setTextColor(TFT_WHITE, TFT_BLACK); }
        break; }
      default:
        break;
    }
    _escState = false; _csi = false; _escBuf = "";
  }

  static int parseOne(const String& s, int defVal) {
    if (s.length() == 0) return defVal;
    return s.toInt();
  }
  static void parseTwo(const String& s, int& a, int& b) {
    int sep = s.indexOf(';');
    if (sep < 0) { a = s.length()? s.toInt():1; b = 1; return; }
    a = (sep? s.substring(0, sep).toInt():1);
    b = ((sep < (int)s.length()-1)? s.substring(sep+1).toInt():1);
  }
};
