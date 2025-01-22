#include <Windows.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include <Artino_Menu.h>
#include "NESpEmulator.h"
#include "..\src\nofrendo-esp32\lcd.h"
#include "..\src\nofrendo-esp32\psxcontroller.h"

#define RGB16(r, g, b) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
#define YELLOW RGB16(255, 255, 0)
#define BLUE RGB16(0, 0, 255)
#define RED RGB16(255, 0, 0)
#define GREEN RGB16(0, 255, 0)
#define WHITE RGB16(255, 255, 255)
#define BLACK RGB16(0, 0, 0)

void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);
void LCD_SetTextColor(uint16_t c, uint16_t bk);
int psxReadInput();
uint32_t LCD_Write(int x, int y, const char* text, size_t len);


class Menu : public Artino::Menu<std::string>
{
    uint32_t m_dwPrevKey = 0xffffffff;
    uint32_t m_dwCurrKey = 0xffffffff;
    typedef std::string MENU_ITEM;
public:
    Menu(const std::string* items, uint16_t count, const Artino::RECT* lprc)
        : Artino::Menu<std::string>(items, count, 16, lprc)
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
            else if ((m_dwPrevKey & (1 << KEYSHIFT_START)) && (m_dwCurrKey & (1 << KEYSHIFT_START)) == 0)
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

        size_t n = pItem->length();
        uint32_t xy_pos = LCD_Write(x, y, pItem->c_str(), -1);
        if ((xy_pos >> 16) == y)
        {
            LCD_Fill(xy_pos & 0xffff, y, 320, y + 16, clrBackground);
        }
    }

    //void OnEndDrawItem() override
    //{
    //}
};

size_t GetFileList(std::vector<std::string>& list)
{
    WIN32_FIND_DATA wfd = { 0 };
    WCHAR buffer[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, buffer);
    PathCombine(buffer, buffer, L"*");

    HANDLE hFind = FindFirstFile(buffer, &wfd);

    if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::wstring strExtension = PathFindExtension(wfd.cFileName);
            std::transform(strExtension.begin(), strExtension.end(), strExtension.begin(), ::tolower);
            if (strExtension != L".nes")
            {
                continue;
            }
            
            char strFilenameUtf8[MAX_PATH];
            WideCharToMultiByte(CP_UTF8, 0, wfd.cFileName, -1, strFilenameUtf8, sizeof(strFilenameUtf8), nullptr, nullptr);
            list.push_back(strFilenameUtf8);
            //if (list.size() >= 3) // test
            //{
            //    break;
            //}
        } while (FindNextFile(hFind, &wfd));
        FindClose(hFind);
    }

    return list.size();
}

const unsigned char* OSDReadFile(const char* szFilename)
{
    //char strFileAnsi[MAX_PATH];
    WCHAR szFileWide[MAX_PATH];
    int len = MultiByteToWideChar(CP_UTF8, 0, szFilename, strlen(szFilename), szFileWide, sizeof(szFileWide));
    szFileWide[len] = L'\0';
    //WideCharToMultiByte(CP_, 0, wfd.cFileName, -1, buffer, sizeof(buffer) / sizeof(buffer[0]), nullptr, nullptr);

    std::fstream file(szFileWide, std::ios::in | std::ios::binary);
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

std::string& BrowseFile(std::string& strFilepath)
{
    Artino::RECT rect = { 0, 0, SCREEN_W, SCREEN_H };
    std::string str = "sdkals skodo";
    std::vector<std::string> filelist;
    GetFileList(filelist);
    Menu menu(&filelist.front(), filelist.size(), &rect);
    int select = menu.Loop();
    LCD_Fill(0, 0, SCREEN_W, SCREEN_H, 0);
    strFilepath = filelist[select];
    return strFilepath;
}

