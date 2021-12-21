#ifndef __VS23S0x0_H__
#define __VS23S0x0_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#ifndef bool
# include <stdbool.h>
#endif

struct video_mode_t {
  uint16_t x;
  uint16_t y;
  uint16_t top;
  uint16_t left;
  uint8_t vclkpp;
  uint8_t bextra;
};

extern const struct video_mode_t *m_current_mode;

#define XPIXELS (m_current_mode->x)
#define YPIXELS (m_current_mode->y)

inline uint16_t width (void)
{
  return XPIXELS;
}

inline uint16_t height (void)
{
  return YPIXELS;
}

void SpiRamWriteRegister (uint16_t, uint16_t);
uint16_t SpiRamReadRegister (uint16_t);

uint16_t currentLine();

void setColorSpace(uint8_t palette);

bool setMode(uint8_t);
void setSyncLine(uint16_t line);

void videoBegin (bool, bool, uint8_t);
void videoInit (uint8_t);
void SetLineIndex(uint16_t line, uint16_t wordAddress);
void SetPicIndex(uint16_t line, uint32_t byteAddress, uint16_t protoAddress);
void setBorder(uint8_t y, uint8_t uv);

void setPixelYuv(uint16_t, uint16_t, uint8_t);
void setPixelRgb(uint16_t, uint16_t, uint8_t, uint8_t, uint8_t);
void clearScreen (uint8_t colour);

void MoveBlock (uint16_t, uint16_t, uint16_t, uint16_t, uint8_t,
		uint8_t, uint8_t);
void blitRect (uint16_t, uint16_t, uint16_t, uint16_t, uint8_t,
	       uint8_t);
void fillRectangle (uint16_t, uint16_t, uint16_t, uint16_t, uint8_t);
void reset(void);

#ifdef __cplusplus
}
#endif

#endif
