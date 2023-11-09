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

#define RGB16(r, g, b) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
#define YELLOW RGB16(255, 255, 0)
#define BLUE RGB16(0, 0, 255)
#define RED RGB16(255, 0, 0)
#define GREEN RGB16(0, 255, 0)
#define WHITE RGB16(255, 255, 255)
#define BLACK RGB16(0, 0, 0)
//#define RGB32to16


struct MENU_ITEM
{
  std::string str;
};

class Menu : public Artino::Menu<MENU_ITEM>
{
  uint32_t m_dwPrevKey = 0xffffffff;
  uint32_t m_dwCurrKey = 0xffffffff;
public:
  Menu(const MENU_ITEM* items, uint16_t count, const Artino::RECT* lprc)
    : Artino::Menu<MENU_ITEM>(items, count, 16, lprc)
  {
  }

  Artino::MenuKey GetKey() override
  {
    uint32_t time_elapse = 0;
    while (1)
    {
      m_dwPrevKey = m_dwCurrKey;
      m_dwCurrKey = psxReadInput();

      if ((m_dwPrevKey & (1 << KEYSHIFT_UP)) && (m_dwCurrKey & (1 << KEYSHIFT_UP)) == 0)
      {
        return Artino::MenuKey_Up;
      }
      else if ((m_dwPrevKey & (1 << KEYSHIFT_DOWN)) && (m_dwCurrKey & (1 << KEYSHIFT_DOWN)) == 0)
      {
        return Artino::MenuKey_Down;
      }
      else if ((m_dwPrevKey & (1 << KEYSHIFT_RIGHT)) && (m_dwCurrKey & (1 << KEYSHIFT_RIGHT)) == 0)
      {
        return Artino::MenuKey_Confirm;
      }

      delay(10);
      time_elapse += 10;
      if (time_elapse > 150 && m_dwPrevKey != 0xffffffff)
      {
        time_elapse = 0;
        m_dwCurrKey = 0xffffffff;
      }
    }

    return Artino::MenuKey_None;
  }

  void OnDrawItem(int16_t x, int16_t y, const MENU_ITEM* pItem, bool selected) override
  {
    uint16_t clrBackground;
    if (selected)
    {
      LCD_SetTextColor(BLUE, YELLOW);
      clrBackground = YELLOW;
    }
    else
    {
      LCD_SetTextColor(YELLOW, BLUE);
      clrBackground = BLUE;
    }

    size_t n = pItem->str.length();
    uint32_t xy_pos = LCD_Write(x, y, pItem->str.c_str(), -1);
    if ((xy_pos >> 16) == y)
    {
      LCD_Fill(xy_pos & 0xffff, y, 320, y + 16, clrBackground);
    }
  }
};

#ifdef _WIN32

class MyFile : public OSDFile
{
  friend OSDFile* OpenDir(const char* strDir);
  HANDLE hFind = NULL;
  WIN32_FIND_DATA wfd = {0};

  OSDFile* OpenNextFile() override
  {
    BOOL result = FindNextFile(hFind, &wfd);
    if (!result)
    {
      FindClose(hFind);
      hFind = INVALID_HANDLE_VALUE;
      memset(&wfd, 0, sizeof(WIN32_FIND_DATAA));
      return NULL;
    }
    return this;
  }

  const char* name() const override
  {
    static CHAR buffer[1024];
    //lstrcpyn(buffer, wfd.cFileName, sizeof(buffer) / sizeof(buffer[0]));
    WideCharToMultiByte(CP_UTF8, 0, wfd.cFileName, -1, buffer, sizeof(buffer) / sizeof(buffer[0]), nullptr, nullptr);
    return buffer;
  }

  const unsigned char* ReadFile(const char* szFile)
  {
    std::fstream file(szFile, std::ios::in | std::ios::binary);
    //file.open(strFile.c_str(), std::ios_base::in);
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    char* pData = nullptr;
    if (size)
    {
      pData = new char[size];
      file.read(pData, size);
    }
    file.close();
    return reinterpret_cast<unsigned char*>(pData);
  }
};

MyFile file;

OSDFile* OpenDir(const char* strDir)
{
  if (file.hFind)
  {
    FindClose(file.hFind);
  }

  WCHAR buffer[MAX_PATH];
  //memset(&file, 0, sizeof(MyFile));
  GetCurrentDirectory(MAX_PATH, buffer);
  PathCombine(buffer, buffer, L"*");

  file.hFind = FindFirstFile(buffer, &file.wfd);
  return &file;
}

#else

#endif


bool IsNESFilename(const std::string& strFilename)
{
  auto pos = strFilename.rfind('.');
  if (pos == std::string::npos)
  {
    return false;
  }
  return strFilename.compare(pos, 4, ".NES") == 0;
}


//
const unsigned char* osd_getromdata()
{
#if 1
  console.SetTextColor(YELLOW, BLUE);
  console.Clear();
  console.Outputln("osd_getromdata");
  console.Outputln(R"(加载文件列表...)");
  delay(1000);
  //console.DrawWindow(0, 0, 40, 15);
  OSDFile* root = OpenDir("/");

  std::vector<MENU_ITEM> files;
  if (root)
  {
    OSDFile* file = root->OpenNextFile();
    while (file/* && files.size() < 10*/)
    {
      MENU_ITEM item;
      item.str = file->name();
      if (IsNESFilename(item.str))
      {
        files.push_back(item);
        //TRACE(file->name());
      }
      file = root->OpenNextFile();
    }
  }

  TRACE("generate file list [OK]");

 
  if (files.empty())
  {
    TRACE("empty file list");
    console.Outputln("empty file list");
    while (1);
  }

  Artino::RECT rect = { 0, 0, SCREEN_W, SCREEN_H };
  Menu menu(&files.front(), files.size(), &rect);

  TRACE("enter menu loop");
  int select = menu.Loop();

  TRACE("exit menu loop");
  console.SetTextColor(WHITE, BLACK);
  console.Clear();
  return root->ReadFile(files[select].str.c_str());

#else
  return GetDefaultRom();
#endif
}