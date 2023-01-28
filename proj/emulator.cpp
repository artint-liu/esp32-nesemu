#include <Windows.h>
#include <gdiplus.h>
#include <inttypes.h>
#include "NESpEmulator.h"

const unsigned char rom[] = {
#include "rom.h"
};

const unsigned char* osd_getromdata()
{
    return rom;
}

void LCD_Display(const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height, const uint8_t* data)
{
    if (data == NULL)
    {
        return;
    }

    for (int i = 0; i < height; i++)
    {
        for (int n = 0; n < width; n++)
        {
            int index = i * 256 + n;
            DWORD c = (DWORD)data[index];
            DWORD col = (c << 24) | (c << 16) | (c << 8) | c;
            g_pScreenBuffer[i * 256 + n] = col;
        }
    }

    SendMessage(g_hWnd, WM_FLUSHSCREEN, 0, 0);
}

void LCD_Init()
{

}

int psxReadInput()
{
    return 0;
}

void psxcontrollerInit()
{
}

