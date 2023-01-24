#include <Windows.h>
#include <inttypes.h>

const unsigned char rom[] = {
#include "rom.h"
};

const unsigned char* osd_getromdata()
{
    return rom;
}

void LCD_Display(const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height, const uint8_t* data[])
{

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

extern "C"
void* pvPortMalloc(size_t xSize)
{
    return malloc(xSize);
}

extern "C"
void vPortFree(void* pv)
{
    free(pv);
}
