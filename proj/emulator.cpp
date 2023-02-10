#include <Windows.h>
#include <gdiplus.h>
#include <inttypes.h>
#include <shlwapi.h>
#include "NESpEmulator.h"
#include "noftypes.h"
#include "Artino_Font.h"
#include "..\src\nofrendo-esp32\lcd.h"
#include "console.h"

#include <vector>
#include <string>

#include <Artino_Menu.h>

#pragma comment(lib, "Shlwapi.lib")

Console console;

const unsigned char rom[] = {
#include "rom.h"
};

extern uint32 myPalette32[256];
volatile DWORD g_dwKeyPressed = 0;
void LCD_Write(int x, int y, const char* text, size_t len);

#define KEYSHIFT_UP	    4
#define KEYSHIFT_RIGHT  5
#define KEYSHIFT_DOWN	6
#define KEYSHIFT_LEFT	7

#define KEYSHIFT_SELECT 0
#define KEYSHIFT_START  3
#define KEYSHIFT_A      13
#define KEYSHIFT_B      14
#define KEYSHIFT_SOFT_RESET	   12
#define KEYSHIFT_HARD_RESET	   15

#define RGB16(r, g, b) (((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3))
#define YELLOW RGB16(255, 255, 0)
#define BLUE RGB16(0, 0, 255)
//#define RGB32to16
#define RGB16to32(rgb) (((((rgb & 0xf800) >> 11) << 3) | 7) << 16) | (((((rgb & 0x7E0) >> 5) << 2) | 3) << 8) | (((rgb & 0x1f) << 3) | 7)
int psxReadInput();

//class FileList
//{
//    BOOL OpenDir(const char* szDir)
//    {
//        FindFirstFile()
//    }
//
//    BOOL Next()
//    {
//
//    }
//};

class Menu : public Artino::Menu
{
    uint32_t m_dwPrevKey = psxReadInput();
public:
    Menu(const Artino::MENU_ITEM* items, uint16_t count)
        : Artino::Menu(items, count)
    {

    }

    Artino::MenuKey GetKey() override
    {
        uint32_t key = psxReadInput();
        uint32_t dwPrevKey = m_dwPrevKey;
        m_dwPrevKey = key;

        if ((dwPrevKey & (1 << KEYSHIFT_UP)) && (key & (1 << KEYSHIFT_UP)) == 0)
        {
            return Artino::MenuKey_Up;
        }
        else if ((dwPrevKey & (1 << KEYSHIFT_DOWN)) && (key & (1 << KEYSHIFT_DOWN)) == 0)
        {
            return Artino::MenuKey_Down;
        }
        else if ((dwPrevKey & (1 << KEYSHIFT_RIGHT)) && (key & (1 << KEYSHIFT_RIGHT)) == 0)
        {
            return Artino::MenuKey_Confirm;
        }

        return Artino::MenuKey_None;
    }

    void OnDrawItem(int x, int y, const char* text, bool selected) override
    {
        if (selected)
        {
            LCD_SetTextColor(BLUE, YELLOW);
        }
        else
        {
            LCD_SetTextColor(YELLOW, BLUE);
        }
        LCD_Write(x, y, text, -1);
    }
};

struct MyFile
{
    HANDLE hFind;
    WIN32_FIND_DATAA wfd;

    MyFile OpenNextFile()
    {
        BOOL result = FindNextFileA(hFind, &wfd);
        if (!result)
        {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
            memset(&wfd, 0, sizeof(WIN32_FIND_DATAA));
        }
        return *this;
    }

    operator bool() const
    {
        return (hFind != INVALID_HANDLE_VALUE);
    }

    const char* name() const
    {
        return wfd.cFileName;
    }
};

MyFile OpenDir(const char* strDir)
{
    MyFile file = {0};
    char buffer[MAX_PATH];
    memset(&file, 0, sizeof(MyFile));
    GetCurrentDirectoryA(MAX_PATH, buffer);
    PathCombineA(buffer, buffer, "*");

    file.hFind = FindFirstFileA(buffer, &file.wfd);
    return file;
}


const unsigned char* osd_getromdata()
{
    console.SetTextColor(YELLOW, BLUE);
    console.Clear();
    //console.DrawWindow(0, 0, 40, 15);
    delay(1000);
    MyFile root = OpenDir("/");
    
    std::vector<std::string> files;
    if (root)
    {
        MyFile file = root.OpenNextFile();
        while (file)
        {
            files.push_back(file.name());
            file = root.OpenNextFile();
            //delay(100);
        }
    }

    std::vector<Artino::MENU_ITEM> filesptr;
    for (std::string& item : files)
    {
        Artino::MENU_ITEM mi;
        mi.text = item.c_str();
        filesptr.push_back(mi);
    }

    Menu menu(&filesptr.front(), filesptr.size());

    int select = menu.Loop();


    //int index = 0;
    //int select = 0;
    //for (auto str : files)
    //{
    //    if (index == select)
    //    {
    //        console.SetTextColor(BLUE, YELLOW);
    //    }
    //    else
    //    {
    //        console.SetTextColor(YELLOW, BLUE);
    //    }

    //    console.Outputln(str.c_str());
    //    delay(200);
    //    index++;
    //}


    //console.Outputln("Hello world");
    //console.Outputln("Can not find SD");
    //for (int i = 0; i < 20; i++)
    //{
    //    char buffer[128];
    //    sprintf(buffer, "string %2d %*c", i, i + 1, '#');
    //    console.Outputln(buffer);
    //    delay(200);
    //}
    //delay(1000);
    console.Clear();
    return rom;
}

void LCD_Scroll(int16_t offsetX, int16_t offsetY)
{
    int x0 = offsetX > 0 ? offsetX : 0;
    int x1 = (LCD_W + offsetX) > LCD_W ? LCD_W : (LCD_W + offsetX);

    int y0 = offsetY > 0 ? offsetY : 0;
    int y1 = (LCD_H + offsetY) > LCD_H ? LCD_H : (LCD_H + offsetY);

    int len = x1 - x0;
    for (int y = y0; y < y1; y++)
    {
        int idxDst = y * LCD_W + x0;
        int idxSrc = (y - offsetY) * LCD_W + (x0 - offsetX);
        memcpy(g_pScreenBuffer + idxDst, g_pScreenBuffer + idxSrc, len * sizeof(DWORD));
    }
}

void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
{
    uint32_t col = RGB16to32(color);
    for (int y = ysta; y < yend; y++)
    {
        int index = y * LCD_W;
        for (int x = xsta; x < xend; x++)
        {
            g_pScreenBuffer[index + x] = col;
        }
    }
    SendMessage(g_hWnd, WM_FLUSHSCREEN, 0, 0);
}

void LCD_Display(const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height, const uint8_t* data)
{
    if (data)
    {
#if 1
        for (int i = 0; i < height; i++)
        {
            for (int n = 0; n < width; n++)
            {
                int index = i * 256 + n;
                if (g_pScreenBuffer)
                {
                    g_pScreenBuffer[(LCD_H - (n + x) - 1) * LCD_W + (i + y)] = myPalette32[data[index]];
                }
            }
        }
#else
        for (int i = 0; i < height; i++)
        {
            for (int n = 0; n < width; n++)
            {
                int index = i * 256 + n;
                g_pScreenBuffer[(n + x) * SCREEN_WIDTH + (i + y)] = myPalette32[data[index]];
            }
        }
#endif
    }

    SendMessage(g_hWnd, WM_FLUSHSCREEN, 0, 0);
}

void LCD_Init()
{

}

int psxReadInput()
{
    return (int)~g_dwKeyPressed;
}

void psxcontrollerInit()
{
}


void KeyCallback(int key, int action)
{
    if (action == WM_KEYDOWN)
    {
        switch (key)
        {
        case VK_UP:     g_dwKeyPressed |= (1 << KEYSHIFT_UP); break;
        case VK_LEFT:   g_dwKeyPressed |= (1 << KEYSHIFT_LEFT); break;
        case VK_DOWN:   g_dwKeyPressed |= (1 << KEYSHIFT_DOWN); break;
        case VK_RIGHT:  g_dwKeyPressed |= (1 << KEYSHIFT_RIGHT); break;
        case 'A':       g_dwKeyPressed |= (1 << KEYSHIFT_A); break;
        case 'S':       g_dwKeyPressed |= (1 << KEYSHIFT_B); break;
        case 'E':       g_dwKeyPressed |= (1 << KEYSHIFT_SELECT); break;
        case 'R':       g_dwKeyPressed |= (1 << KEYSHIFT_START); break;
        case 'O':       g_dwKeyPressed |= (1 << KEYSHIFT_SOFT_RESET); break;
        case 'P':       g_dwKeyPressed |= (1 << KEYSHIFT_HARD_RESET); break;
        }
    }
    else if (action == WM_KEYUP)
    {
        switch (key)
        {
        case VK_UP:     g_dwKeyPressed &= (~(1 << KEYSHIFT_UP)); break;
        case VK_LEFT:   g_dwKeyPressed &= (~(1 << KEYSHIFT_LEFT)); break;
        case VK_DOWN:   g_dwKeyPressed &= (~(1 << KEYSHIFT_DOWN)); break;
        case VK_RIGHT:  g_dwKeyPressed &= (~(1 << KEYSHIFT_RIGHT)); break;
        case 'A':       g_dwKeyPressed &= (~(1 << KEYSHIFT_A)); break;
        case 'S':       g_dwKeyPressed &= (~(1 << KEYSHIFT_B)); break;
        case 'E':       g_dwKeyPressed &= (~(1 << KEYSHIFT_SELECT)); break;
        case 'R':       g_dwKeyPressed &= (~(1 << KEYSHIFT_START)); break;
        case 'O':       g_dwKeyPressed &= (~(1 << KEYSHIFT_SOFT_RESET)); break;
        case 'P':       g_dwKeyPressed &= (~(1 << KEYSHIFT_HARD_RESET)); break;
        }
    }
}


void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x < LCD_W && y < LCD_H)
    {
        int index = (y * LCD_W + x);
        if (g_pScreenBuffer)
        {
            g_pScreenBuffer[index] = RGB16to32(color);
        }
    }
}


uint32_t crText = 0, crTextBk = 0xffffffff;
void LCD_SetTextColor(uint16_t c, uint16_t bk)
{
    crText = c;
    crTextBk = bk;
}

void DrawChar(int x, int y, int w, int h, const uint8_t* data)
{
    //if (m_clrText)
    {
        for (int yy = 0; yy < h; yy++)
        {
            for (int xx = 0; xx < w; xx++)
            {
                int index = yy * w + xx;
                LCD_DrawPixel(y + yy, LCD_H - (x + xx) - 1, (data[index >> 3] & (0x80 >> (index & 7))) ? crText : crTextBk);
            }
        }
    }
}

void LCD_Write(int x, int y, const char* text, size_t len)
{
    Artint_HZK::BITMAPFONT font = { 0 };

    for (size_t i = 0; i < len;)
    {
        if (*text == '\0')
        {
            return;
        }

        int8_t L = Artint_HZK::hzk16.Query(text, len - i, &font);
        if (L <= 0)
        {
            i++;
            text++;
            continue;
        }

        DrawChar(x, y, font.width, font.height, font.data);
        text += L;
        i += L;
        x += font.width;
        if (x + 16 >= LCD_H)
        {
            x = 0;
            y += font.height;
            if (y >= LCD_W)
            {
                return;
            }
        }
    }
}

void LCD_WriteASCII(int x, int y, const char* text, size_t len)
{
    Artint_HZK::BITMAPFONT font = { 0 };

    for (size_t i = 0; i < len;)
    {
        if (*text == '\0')
        {
            return;
        }

        int8_t L = Artint_HZK::asc16.Query(text, len - i, &font);
        if (L <= 0)
        {
            i++;
            text++;
            continue;
        }

        DrawChar(x, y, font.width, font.height, font.data);
        text += L;
        i += L;
        x += font.width;
        if (x + 8 > LCD_H)
        {
            x = 0;
            y += font.height;
            if (y >= LCD_W)
            {
                return;
            }
        }
    }
}
