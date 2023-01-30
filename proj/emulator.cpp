#include <Windows.h>
#include <gdiplus.h>
#include <inttypes.h>
#include "NESpEmulator.h"
#include "noftypes.h"

const unsigned char rom[] = {
#include "rom.h"
};

extern uint32 myPalette32[256];
volatile DWORD g_dwKeyPressed = 0;

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

#if 1
    for (int i = 0; i < height; i++)
    {
        for (int n = 0; n < width; n++)
        {
            int index = i * 256 + n;
            g_pScreenBuffer[(SCREEN_HEIGHT - (i + y) - 1) * SCREEN_WIDTH + n + x] = myPalette32[data[index]];
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