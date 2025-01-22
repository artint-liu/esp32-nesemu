#ifdef _WIN32
#include <Windows.h>
#include <shlwapi.h>
#define delay Sleep
#else
#include <Arduino.h>
#include <cstring>
#endif
#include <vector>
#include <string>
#include <fstream>
#include <Artino_Menu.h>
#include <Artino_Console.h>
#include "nofrendo-esp32\psxcontroller.h"
#include "console.h"
#include "osd.h"
#include "nofrendo-esp32\lcd.h"

void LCD_SetTextColor(uint16_t c, uint16_t bk);
extern Console console;

const uint8_t* GetDefaultRom();


bool IsNESFilename(const std::string& strFilename)
{
  auto pos = strFilename.rfind('.');
  if (pos == std::string::npos)
  {
    return false;
  }
  return strFilename.compare(pos, 4, ".NES") == 0;
}

const unsigned char* ReadFile(const char* szFilename);

//
const unsigned char* osd_getromdata(const char* filename)
{
    if (!filename)
    {
        return GetDefaultRom();
    }
    else
    {
        return OSDReadFile(filename);
    }
}