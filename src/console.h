#ifndef _ARTINT_CONSOLE_H_
#define _ARTINT_CONSOLE_H_

class Console
{
    int16_t m_x = 0;
    int16_t m_y = 0;
    uint16_t m_crText = 0XFFFF;
    uint16_t m_crTextBk = 0x0000;
public:
    void Clear();
    void SetTextColor(uint16_t crText, uint16_t crBk);
    void Outputln(const char* szText);
    void DrawWindow(int x, int y, int w, int h); // (0,0) - (40, 15)
    void Scroll(int16_t offsetX, int16_t offsetY);
};

#endif // _ARTINT_CONSOLE_H_