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

typedef enum
{
  TMS_TRANSPARENT = -1,
  TMS_BLACK = 0,
  TMS_MED_GREEN = 8*16 + 5,
  TMS_LT_GREEN = 8*16 + 9,
  TMS_DK_BLUE = 4*16 + 3,
  TMS_LT_BLUE = 4*16 + 9,
  TMS_DK_RED = 14*16,
  TMS_CYAN = 5*16 + 8,
  TMS_MED_RED = 14*16 + 5,
  TMS_LT_RED = 14*16 + 9,
  TMS_DK_YELLOW = 11*16,
  TMS_LT_YELLOW = 10*16,
  TMS_DK_GREEN = 12*16,
  TMS_MAGENTA = 6*16+6,
  TMS_GREY = 10,
  TMS_WHITE = 15,
} vrEmuTms9918aColor;

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
  clearScreen (bgColor);
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
		setPixelYuv (tileX * TEXT_CHAR_WIDTH + i, y, fgColor);
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

/* Function:  tms9918aScanLine
 * ----------------------------------------
 * generate a scanline
 */
void
tms9918aDisplay (void)
{
  switch (tms9918a.mode)
    {
    case TMS_MODE_GRAPHICS_I:
      //vrEmuTms9918aGraphicsIScanLine(tms9918a, y, pixels);
      break;

    case TMS_MODE_GRAPHICS_II:
      //vrEmuTms9918aGraphicsIIScanLine(tms9918a, y, pixels);
      break;

    case TMS_MODE_TEXT:
      tms9918aTextMode ();
      break;

    case TMS_MODE_MULTICOLOR:
      //vrEmuTms9918aMulticolorScanLine(tms9918a, y, pixels);
      break;
    }
}

void
tms9918aInit (void)
{
  tms9918a.lastMode = 0;
  tms9918a.currentAddress = 0;
  tms9918a.mode = TMS_MODE_TEXT;
  videoBegin (false, false, 1);
}
