#include <Windows.h>
#include <gdiplus.h>
#include <inttypes.h>
#include "NESpEmulator.h"
#include "noftypes.h"

const unsigned char rom[] = {
#include "rom.h"
};

extern uint32 myPalette32[256];

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
            g_pScreenBuffer[(SCREEN_HEIGHT - (i + y) - 1) * SCREEN_WIDTH + n + x] = myPalette32[data[index]];
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

