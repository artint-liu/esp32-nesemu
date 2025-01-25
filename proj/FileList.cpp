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
        m_dwCurrKey = psxReadInput();
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
};



size_t GetFileList(const char* rootdir, std::vector<std::string>& list)
{
    WIN32_FIND_DATA wfd = { 0 };
    WCHAR buffer[MAX_PATH];
    WCHAR rootdirW[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, rootdir, -1, rootdirW, MAX_PATH);

    GetCurrentDirectory(MAX_PATH, buffer);
    PathCombine(buffer, buffer, rootdirW);
    PathCombine(buffer, buffer, L"*");

    HANDLE hFind = FindFirstFile(buffer, &wfd);

    if (hFind != NULL && hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::wstring strExtension = PathFindExtension(wfd.cFileName);
            std::transform(strExtension.begin(), strExtension.end(), strExtension.begin(), ::tolower);
            if (lstrcmpW(wfd.cFileName, L".") == 0)
            {
                continue;
            }
            else if (strExtension != L".nes" && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                continue;
            }


            
            char strFilenameUtf8[MAX_PATH];
            WideCharToMultiByte(CP_UTF8, 0, wfd.cFileName, -1, strFilenameUtf8, sizeof(strFilenameUtf8), nullptr, nullptr);
            list.push_back(strFilenameUtf8);
            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                list.back().insert(0, "<");
                list.back().push_back('>');
            }
        } while (FindNextFile(hFind, &wfd));
        FindClose(hFind);
    }

    return list.size();
}

const unsigned char* OSDReadFile(const char* szFilename)
{
    //char strFileAnsi[MAX_PATH];
    CHAR szFullPath[MAX_PATH];
    CHAR buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);

    //int len = MultiByteToWideChar(CP_UTF8, 0, szFilename, strlen(szFilename), szFileWide, sizeof(szFileWide));
    //szFileWide[len] = L'\0';

    PathCombineA(szFullPath, buffer, szFilename);

    //WideCharToMultiByte(CP_, 0, wfd.cFileName, -1, buffer, sizeof(buffer) / sizeof(buffer[0]), nullptr, nullptr);

    std::fstream file(szFullPath, std::ios::in | std::ios::binary);
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

std::string& CombinPath(std::string& strDir, const std::string& strFile)
{
    if (strDir.back() != '/')
    {
        strDir.push_back('/');
    }
    strDir += strFile;
    return strDir;
}

std::string& RemoveLastDir(std::string& strDir)
{
    size_t pos = strDir.rfind('/');
    if (pos != std::string::npos)
    {
        if (pos > 0)
        {
            strDir = strDir.substr(0, pos);
        }
        else
        {
            strDir = "/";
        }
    }
    return strDir;
}

std::string Utf8ToAnsi(const std::string& strUtf8)
{
    const size_t buffer_size = 1024;
    WCHAR bufferW[buffer_size] = { 0 };
    CHAR buffer[buffer_size] = {0};
    MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), strUtf8.size(), bufferW, buffer_size);
    WideCharToMultiByte(CP_ACP, 0, bufferW, buffer_size, buffer, buffer_size, NULL, FALSE);
    return buffer;
}

std::string& BrowseFile(std::string& strFilepath)
{
    Artino::RECT rect = { 0, 0, SCREEN_W, SCREEN_H };
    std::string strDir = "/";
    const char8_t* szLoading = u8"文件列表读取中";

    while (true)
    {
        //delay(100);
        std::vector<std::string> filelist;
        GetFileList(strDir.c_str(), filelist);
        LCD_Fill(0, 0, SCREEN_W, SCREEN_H, 0);
        delay(10); // 不延时显示不完整

        Menu menu(&filelist.front(), filelist.size(), &rect);
        int select = menu.Loop();
        LCD_Fill(0, 0, SCREEN_W, SCREEN_H, 0);
        strFilepath = filelist[select];
        if (strFilepath == "<..>")
        {
            RemoveLastDir(strDir);
        }
        else if (strFilepath.front() == '<')
        {
            strFilepath.erase(strFilepath.begin());
            strFilepath.erase(strFilepath.end() - 1);
            CombinPath(strDir, strFilepath);
        }
        else
        {
            CombinPath(strDir, strFilepath);
            break;
        }
        LCD_Write(100, 120 - 16, (const char*)szLoading, strlen((const char*)szLoading));
        //LCD_Flush();
    }

    strFilepath = Utf8ToAnsi(strDir);
    return strFilepath;
}

