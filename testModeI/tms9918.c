#include "vs23s0x0.h"
#include "tms9918.h"

#define VRAM_SIZE (1 << 13) /* 8k 16KB */

#define NUM_REGISTERS 8

#define GRAPHICS_NUM_COLS 32
#define GRAPHICS_NUM_ROWS 24
#define GRAPHICS_CHAR_WIDTH 8

#define TEXT_NUM_COLS 40
#define TEXT_NUM_ROWS 24
#define TEXT_CHAR_WIDTH 6

#define ANY_CHAR_HEIGHT 8

#define MAX_SPRITES 32
#define SPRITE_ATTR_BYTES 4
#define LAST_SPRITE_VPOS 0xD0
#define MAX_SCANLINE_SPRITES 4

 /* PRIVATE DATA STRUCTURE
  * ---------------------------------------- */

typedef enum
{
  TMS_MODE_GRAPHICS_I,
  TMS_MODE_GRAPHICS_II,
  TMS_MODE_TEXT,
  TMS_MODE_MULTICOLOR,
} vrEmuTms9918aMode;


/*
 * TMS9918 RGB colors
 * Color code     Color         R       G       B
 * 1              black         00      00      00
 * 2              medium green  0A      AD      1E
 * 3              light green   34      C8      4C
 * 4              dark blue     2B      2D      E3
 * 5              light blue    51      4B      FB
 * 6              dark red      BD      29      25
 * 7              cyan          1E      E2      EF
 * 8              medium red    FB      2C      2B
 * 9              light red     FF      5F      4C
 * 10             dark yellow   BD      A2      2B
 * 11             light yellow  D7      B4      54
 * 12             dark green    0A      8C      18
 * 13             magenta       AF      32      9A
 * 14             gray          B2      B2      B2
 * 15             white         FF      FF      FF
 */

typedef enum
{
  TMS_TRANSPARENT,
  TMS_BLACK,
  TMS_MED_GREEN,
  TMS_LT_GREEN,
  TMS_DK_BLUE,
  TMS_LT_BLUE,
  TMS_DK_RED,
  TMS_CYAN,
  TMS_MED_RED,
  TMS_LT_RED,
  TMS_DK_YELLOW,
  TMS_LT_YELLOW,
  TMS_DK_GREEN,
  TMS_MAGENTA,
  TMS_GREY,
  TMS_WHITE
} vrEmuTms9918aColor;

static const uint8_t colorLUT[] = {200, 0, 22, 24, 84, 70, 164, 90, 166, 168, 169, 171,
  21, 38, 8, 15};

#define TMS9918A_PIXELS_X 256
#define TMS9918A_PIXELS_Y 192

static struct vrEmuTMS9918a_s
{
  /* TMS9918 VRAM 16Kb*/
  uint8_t vram[VRAM_SIZE];

  uint8_t registers[NUM_REGISTERS];

  uint8_t lastMode;

  uint32_t currentAddress; /* 16b address */

  vrEmuTms9918aMode mode;
} tms9918a;


/* Function:  tmsMode
 * --------------------
 * return the current mode
 */
static vrEmuTms9918aMode tmsMode (void)
{
  if (tms9918a.registers[0] & 0x02)
  {
    return TMS_MODE_GRAPHICS_II;
  }

  switch ((tms9918a.registers[1] & 0x18) >> 3)
  {
    case 0:
      return TMS_MODE_GRAPHICS_I;

    case 1:
      return TMS_MODE_MULTICOLOR;

    case 2:
      return TMS_MODE_TEXT;
  }
  return TMS_MODE_GRAPHICS_I;
}

/* Function:  tmsPatternTableAddr
  * --------------------
  * pattern table base address
  */
static inline uint32_t
tmsPatternTableAddr (void)
{
  if (tms9918a.mode == TMS_MODE_GRAPHICS_II)
    return (tms9918a.registers[4] & 0x04) << 11;
  return (tms9918a.registers[4] & 0x07) << 11;
}

/* Function:  tmsNameTableAddr
  * --------------------
  * name table base address
  */
static inline uint32_t
tmsNameTableAddr (void)
{
  return (tms9918a.registers[2] & 0x0f) << 10;
}

/* Function:  tmsBgColor
  * --------------------
  * background color
  */
static inline vrEmuTms9918aColor
tmsMainBgColor (void)
{
  return (vrEmuTms9918aColor)(tms9918a.registers[7] & 0x0f);
}

/* Function:  tmsFgColor
  * --------------------
  * foreground color
  */
static inline vrEmuTms9918aColor
tmsMainFgColor (void)
{
  vrEmuTms9918aColor c = (vrEmuTms9918aColor)(tms9918a.registers[7] >> 4);
  return c == TMS_TRANSPARENT ? tmsMainBgColor() : c;
}

/* Function:  tms9918aTextMode
 * ----------------------------------------
 * generate a full Text mode screen
 */

static void
tms9918aTextMode (void)
{
  vrEmuTms9918aColor bgColor = tmsMainBgColor ();
  vrEmuTms9918aColor fgColor = tmsMainFgColor ();

  //FIXME Set BG
  clearScreen (colorLUT[bgColor]);
  for (uint16_t y = 0;  y < height(); y++)
    {
      int textRow = y / 8;
      int patternRow = y % 8;
      uint32_t namesAddr = tmsNameTableAddr() + textRow * TEXT_NUM_COLS;

      for (uint16_t tileX = 0; tileX < TEXT_NUM_COLS; tileX++) //x.vs23.width()
	{
	  uint8_t pattern = tms9918a.vram[namesAddr + tileX];
	  uint8_t patternByte = tms9918a.vram[tmsPatternTableAddr() +
					      pattern * 8 + patternRow];

	  for (int i = 0; i < TEXT_CHAR_WIDTH; i++)
	    {
	      //EXTEND: Each Char has it's own BG & effects.
	      if (patternByte & 0x80) // BG is set globaly
		setPixelYuv (tileX * TEXT_CHAR_WIDTH + i + 8, y,
			     colorLUT[fgColor]);
	      patternByte <<= 1;
	    }
	}
    }
}

static void
tms99918aTextUpdate (uint8_t pattern, uint8_t patternRow, bool updateChar)
{
  vrEmuTms9918aColor fgColor = tmsMainFgColor ();
  uint32_t tileY = (pattern / TEXT_NUM_COLS) * ANY_CHAR_HEIGHT;
  uint32_t tileX = (pattern % TEXT_NUM_COLS) * TEXT_CHAR_WIDTH;
  uint8_t j = updateChar ? 0 : patternRow;

  do
    {
      uint8_t patternByte = tms9918a.vram[tmsPatternTableAddr() +
					  pattern * 8 + j];
      j++;
      for (int i = 0; i < TEXT_CHAR_WIDTH; i++)
	{
	  if (patternByte & 0x80) // BG is set globaly
	    setPixelYuv (tileX + i, tileY + j, fgColor);
	  patternByte <<= 1;
	}
    } while (updateChar && (j < ANY_CHAR_HEIGHT));
}

/* Function:  tms9918aRegValue
 * ----------------------------------------
 * return a reigister value
 */
uint8_t
tms9918aRegValue (uint8_t reg)
{
  return tms9918a.registers[reg & 0x07];
}

/* Function:  tms9918aVramValue
 * ----------------------------------------
 * return a value from vram
 */
uint8_t
tms9918aVramValue (uint32_t addr)
{
  return tms9918a.vram[addr & 0x3fff];
}

/* Function:  tms9918aReadData
 * --------------------
 * write data (mode = 0) to the tms9918a
 *
 * byte: the data (DB0 -> DB7) to send
 */
uint8_t
tms9918aReadData (void)
{
  return tms9918a.vram[(tms9918a.currentAddress++) & 0x3fff];
}

/* Function:  tms9918aWriteAddr
 * --------------------
 */
void
tms9918aWriteAddr(uint16_t data)
{
  tms9918a.currentAddress = data & 0x3fff;
}

void
tms9918aWriteReg(uint8_t regno, uint8_t data)
{
  tms9918a.registers[regno & 0x07] = data;
}

/* Function:  vrEmuTms9918aWriteData
 * --------------------
 * write data (mode = 0) to the tms9918a
 *
 * byte: the data (DB0 -> DB7) to send
 */
void
tms9918aWriteData (uint8_t data)
{
  tms9918a.vram[tms9918a.currentAddress] = data;

#if 0
  /* 1. Check if we change a Name table entry.  Update the given
     address.  */
  if (tms9918a.currentAddress >= tmsNameTableAddr()
      && tms9918a.currentAddress < (tmsNameTableAddr() + 1024))
    tms9918aTextUpdate (data, 0, true); /* patternRow doesn't count.  */
#endif

  tms9918a.currentAddress = (tms9918a.currentAddress + 1) & 0x3fff;
}

/* Function:  tmsFgColor
  * --------------------
  * foreground color
  */
static inline vrEmuTms9918aColor
tmsFgColor(uint8_t colorByte)
{
  vrEmuTms9918aColor c = (vrEmuTms9918aColor)(colorByte >> 4);
  return c == TMS_TRANSPARENT ? tmsMainBgColor() : c;
}

/* Function:  tmsBgColor
  * --------------------
  * background color
  */
static inline vrEmuTms9918aColor
tmsBgColor(uint8_t colorByte)
{
  vrEmuTms9918aColor c = (vrEmuTms9918aColor)(colorByte & 0x0f);
  return c == TMS_TRANSPARENT ? tmsMainBgColor() : c;
}


/* Function:  vrEmuTms9918aMulticolorScanLine
 * ----------------------------------------
 * generate a Multicolor mode scanline
 */
static void
tms9918aMulticolorMode (void)
{
  for (uint16_t y = 0;  y < height(); y++)
    {
      int pixelIndex = 0;
      int textRow = y / 8;
      int patternRow = (y / 4) % 2 + (textRow % 4) * 2;
      unsigned short namesAddr = tmsNameTableAddr() +
	textRow * GRAPHICS_NUM_COLS;

      for (int tileX = 0; tileX < GRAPHICS_NUM_COLS; tileX++)
	{
	  int pattern = tms9918a.vram[namesAddr + tileX];

	  uint8_t colorByte = tms9918a.vram[tmsPatternTableAddr() +
					    pattern * 8 + patternRow];

	  for (int i = 0; i < 4; ++i)
	    setPixelYuv (pixelIndex++, y, colorLUT[tmsFgColor(colorByte)]);
	  for (int i = 0; i < 4; ++i)
	    setPixelYuv (pixelIndex++, y, colorLUT[tmsBgColor(colorByte)]);
	}
    }

  //FIXME vrEmuTms9918aOutputSprites(tms9918a, y, pixels);
}

/* Function:  tmsColorTableAddr
  * --------------------
  * color table base address
  */
static inline unsigned short tmsColorTableAddr(void)
{
  if (tms9918a.mode == TMS_MODE_GRAPHICS_II)
    return (tms9918a.registers[3] & 0x80) << 6;
  return tms9918a.registers[3] << 6;
}

/* Function:  vrEmuTms9918aGraphicsIScanLine
 * ----------------------------------------
 * generate a Graphics I mode scanline
 */
static void
tms9918aGraphicsIMode (void)
{
  unsigned short patternBaseAddr = tmsPatternTableAddr();
  unsigned short colorBaseAddr = tmsColorTableAddr();

  for (uint16_t y = 0;  y < height(); y++)
    {
      int textRow = y / 8;
      int patternRow = y % 8;

      unsigned short namesAddr = tmsNameTableAddr() +
	textRow * GRAPHICS_NUM_COLS;

      int pixelIndex = 0;

      for (int tileX = 0; tileX < GRAPHICS_NUM_COLS; tileX++)
	{
	  int pattern = tms9918a.vram[namesAddr + tileX];

	  uint8_t patternByte = tms9918a.vram[patternBaseAddr +
					      pattern * 8 + patternRow];

	  uint8_t colorByte = tms9918a.vram[colorBaseAddr + pattern / 8];

	  uint8_t fgColor = colorLUT[tmsFgColor(colorByte)];
	  uint8_t bgColor = colorLUT[tmsBgColor(colorByte)];

	  for (int i = 0; i < GRAPHICS_CHAR_WIDTH; ++i)
	    {
	      setPixelYuv (pixelIndex++, y,
			   (patternByte & 0x80) ? fgColor : bgColor);
	      patternByte <<= 1;
	    }
	}
    }

  //vrEmuTms9918aOutputSprites(tms9918a, y, pixels);
}

/* Function:  tms9918aScanLine
 * ----------------------------------------
 * generate a scanline
 */
void
tms9918aDisplay (void)
{
  tms9918a.mode = tmsMode();
  switch (tms9918a.mode)
    {
    case TMS_MODE_GRAPHICS_I:
      tms9918aGraphicsIMode ();
      break;

    case TMS_MODE_GRAPHICS_II:
      //vrEmuTms9918aGraphicsIIScanLine(tms9918a, y, pixels);
      break;

    case TMS_MODE_TEXT:
      tms9918aTextMode ();
      break;

    case TMS_MODE_MULTICOLOR:
      tms9918aMulticolorMode ();
      break;
    }
}

void
tms9918aInit (void)
{
  tms9918a.lastMode = 0;
  tms9918a.currentAddress = 0;
  tms9918a.mode = TMS_MODE_TEXT;
  videoBegin (false, true, 1);
}
