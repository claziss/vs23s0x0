/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2021 czi
 * Based on example code by Ulrich Hecht.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#include <SPI.h>
#include "vs_hal.h"
#include "vs23s0x0.h"

static const uint8_t vs23_ops_ntsc[2][5] = {
  {
    /* N-0C-B62-A63-Y33-N10 (NTSC equivalent of P-DD-A62-B63-Y33-N10) */
    PICK_B + PICK_BITS(6) + SHIFT_BITS(2),
    PICK_A + PICK_BITS(6) + SHIFT_BITS(3),
    PICK_Y + PICK_BITS(3) + SHIFT_BITS(3),
    PICK_NOTHING,
    0x0c,
  },
  {
    /* N-0D-B22-A22-Y44-N10 (NTSC equivalent of P-EE-A22-B22-Y44-N10) */
    PICK_B + PICK_BITS(2) + SHIFT_BITS(2),
    PICK_A + PICK_BITS(2) + SHIFT_BITS(2),
    PICK_Y + PICK_BITS(4) + SHIFT_BITS(4),
    PICK_NOTHING,
    0x0d,
  },
};
static const uint8_t vs23_ops_pal[2][5] = {
  {
    /* P-DD-A62-B63-Y33-N10 (PAL equivalent of N-0C-B62-A63-Y33-N10) */
    PICK_A + PICK_BITS(6) + SHIFT_BITS(2),
    PICK_B + PICK_BITS(6) + SHIFT_BITS(3),
    PICK_Y + PICK_BITS(3) + SHIFT_BITS(3),
    PICK_NOTHING,
    0xdd,
  },
  {
    /* P-EE-A22-B22-Y44-N10 (PAL equivalent of N-0D-B22-A22-Y44-N10) */
    PICK_A + PICK_BITS(2) + SHIFT_BITS(2),
    PICK_B + PICK_BITS(2) + SHIFT_BITS(2),
    PICK_Y + PICK_BITS(4) + SHIFT_BITS(4),
    PICK_NOTHING,
    0xee,
  },
};

static const struct video_mode_t modes_ntsc[] = {
  // Maximum usable on NTSC without overscan, 76 6-pixel chars/line,
  // 57 8-pixel chars.
  // NB: This is wider than necessary because a 456-pixel width causes
  // artifacts on PAL, and we want to have the same geometry on both
  // systems.
  {460, 224, 9, 10, 3, 9, 11000000},
  // a bit smaller, may fit better on some TVs; width chosen to work on
  // both PAL and NTSC without artifacts.
  {436, 216, 13, 14, 3, 9, 11000000},
  {320, 216, 11, 15, 4, 9, 11000000},	// VS23 NTSC demo
  {320, 200, 20, 15, 4, 9, 14000000},	// (M)CGA, Commodore et al.
  {256, 224, 9, 15, 5, 9, 15000000},	// SNES
  {256, 192, 24, 15, 5, 8, 11000000},	// MSX, Spectrum, NDS XXX: has
  // artifacts
  {160, 200, 20, 15, 8, 8, 11000000},	// Commodore/PCjr/CPC
  // multi-color
  // overscan modes
  {352, 240, 0, 8, 4, 9, 11000000},	// PCE overscan (is this
  // useful?)
  {282, 240, 0, 8, 5, 8, 11000000},	// PCE overscan (is this
  // useful?)
  // Overscan on NTSC, for compatibility with "maximum PAL" mode.
  {508, 240, 0, 0, 3, 8, 11000000},
};

static const struct video_mode_t modes_pal[] = {
  // Much smaller than the screen, but have to be compatible with NTSC.
  {460, 224, 32, 29, 3, 8, 11000000},
  // This could work with a pixel clock divider of 4, but it would be
  // extremely wide (near overscan).
  {436, 216, 33, 29, 3, 8, 11000000},
  {320, 216, 33, 15, 5, 8, 11000000},	// VS23 NTSC demo
  {320, 200, 41, 15, 5, 8, 14000000},	// (M)CGA, Commodore et al.
  {256, 224, 32, 20, 6, 8, 15000000},	// SNES
  {256, 192, 42, 20, 6, 8, 11000000},	// MSX, Spectrum, NDS
  {160, 200, 41, 15, 10, 8, 11000000},	// Commodore/PCjr/CPC
  // multi-color
  // "Overscan modes" are actually underscan on PAL.
  {352, 240, 24, 8, 5, 8, 11000000},	// PCE overscan (barely)
  {282, 240, 24, 8, 6, 8, 11000000},	// PCE overscan (underscan on PAL)
  // Maximum PAL (the timing would allow more, but we would run out of memory)
  // NB: This does not line up with any font sizes; the width is chosen
  // to avoid artifacts.
  {508, 240, 24, 20, 3, 9, 11000000},
};

struct palette {
  uint32_t rgb;
  uint8_t  yuv;
};

//#include "P-EE-A22-B22-Y44-N10.h"
//#include "N-0C-B62-A63-Y33-N10.h"

#define COLOR(rgb, yuv) {rgb, yuv},
static const struct palette pal[] = {
  #include "P-EE-A22-B22-Y44-N10.h"
};

static inline void vs23Select()
{
  //SpiLock();
  VS23_SELECT;
}

static inline void vs23Deselect()
{
  VS23_DESELECT;
  //SpiUnlock();
}

static inline bool blockFinished (void)
{
  return VS23_MBLOCK;
}

#if 0
static void rgb_to_hsv(uint8_t r, uint8_t g, uint8_t b,
                       int *h, int *s, int *v) {
  *v = max(max(r, g), b);
  if (*v == 0) {
    *s = 0;
    *h = 0;
    return;
  }
  int m = min(min(r, g), b);
  *s = 255 * (*v - m) / *v;
  if (*v == m) {
    *h = 0;
  } else {
    int d = *v - m;
    if (*v == r)
      *h = 60 * (g - b) / d;
    else if (*v == g)
      *h = 120 + 60 * (b - r) / d;
    else
      *h = 240 + 60 * (r - g) / d;
  }
  if (*h < 0)
    *h += 360;
}
#endif

static uint8_t
colorFromRgb (uint8_t r, uint8_t g, uint8_t b)
{
  uint32_t rgb = (r << 16) | (g << 8) | b;
  uint8_t mid = 64;
  uint8_t lo = 0;
  uint8_t hi = 255;


  //1. FIXME! add color correction.
  //2. FIXME! add 8 locations cache algorithm.

  // Not necessary if table's extremes are zero or 0xffffff
  rgb = max (pal[0].rgb, rgb);
  rgb = min (pal[255].rgb, rgb);

  while (hi >= lo)
    {
      mid = lo + ((hi - lo) >> 1);

      uint8_t lent = (mid == 0) ? 0 : mid - 1;
      if (rgb == pal[mid].rgb)
	return pal[mid].yuv;
      else if ((rgb > pal[lent].rgb) && (rgb < pal[mid].rgb))
	return min(pal[mid].yuv - rgb, rgb - pal[lent].rgb);

      if (rgb > pal[mid].rgb)
	lo = mid + 1;
      else
	hi = mid - 1;
    }

  return pal[mid].yuv;
}

// ---------------------------------------------------------------------------
// Equivalent to SPIReadRegister16
// Read 16b register and return value
//
uint16_t
VS23S0x0::SpiRamReadRegister (uint16_t opcode)
{
  uint16_t result;

  vs23Select();
  SPI.transfer(opcode);
#if 1
  result = SPI.transfer(0) << 8;
  result |= SPI.transfer(0);
#else
  result = SPI.transfer16 (0);
#endif
  vs23Deselect();
  return result;
}

// ---------------------------------------------------------------------------
// Equivalent to SPIReadRegister
// Read 8b register and return value
//
static uint8_t ATTRIBUTE_UNUSED
SpiRamReadRegister8 (uint16_t opcode)
{
  uint16_t result;

  vs23Select();
  SPI.transfer(opcode);
  result = SPI.transfer(0);
  vs23Deselect();
  return result;
}

// ---------------------------------------------------------------------------
// Equivalent to SPIWriteRegister16
// Write 16b register
//
void
VS23S0x0::SpiRamWriteRegister (uint16_t opcode, uint16_t data)
{
  // Serial.printf("%02x <= %04xh\n",opcode,data);
  vs23Select();
  SPI.transfer(opcode);
  SPI.transfer(data >> 8);
  SPI.transfer((uint8_t) data);
  vs23Deselect();
}

// ---------------------------------------------------------------------------
// Equivalent to SPIWriteRegister
// Write 8b register
//
static void
SpiRamWriteByteRegister (uint16_t opcode, uint16_t data)
{
  // Serial.printf("%02x <= %02xh\n",opcode,data);
  vs23Select();
  SPI.transfer(opcode);
  SPI.transfer((uint8_t) data);
  vs23Deselect();
}

// ---------------------------------------------------------------------------
// Equivalent to SPIWriteRegister32
// Write 32b register
//
static void
SpiRamWriteProgram (uint16_t opcode, uint16_t data1, uint16_t data2)
{
  // Serial.printf("PROG:%04x%04xh\n",data1,data2);
  vs23Select();
  SPI.transfer(opcode);
  SPI.transfer(data1 >> 8);
  SPI.transfer((uint8_t) data1);
  SPI.transfer(data2 >> 8);
  SPI.transfer((uint8_t) data2);
  vs23Deselect();
}

// ---------------------------------------------------------------------------
// Equivalent to SPIWriteByte
//
static void
SpiRamWriteByte (uint32_t address, uint8_t data)
{
  uint8_t req[5];
  // address = address + (channel * ch_block);

  req[0] = WRITE_SRAM; //0x02
  req[1] = address >> 16;
  req[2] = address >> 8;
  req[3] = address;
  req[4] = data;
  vs23Select();
  //  SPI.writeBytes(req, 5);
  for (int i = 0; i < 5; i++)
    SPI.transfer (req[i]);
  vs23Deselect();
}

// ---------------------------------------------------------------------------
// Equivalent to SPIWriteWord
//
static void
SpiRamWriteWord (uint16_t waddress, uint16_t data)
{
  uint8_t req[6];
  uint32_t address = (uint32_t) waddress << 1;
  // address = address + (channel * ch_block);

  req[0] = WRITE_SRAM; //0x02
  req[1] = address >> 16;
  req[2] = address >> 8;
  req[3] = address;
  req[4] = data >> 8;
  req[5] = data;
  vs23Select();
  //  SPI.writeBytes(req, 6);
  for (int i = 0; i < 6; i++)
    SPI.transfer (req[i]);
  vs23Deselect();
}

// ---------------------------------------------------------------------------
// Equivalent to _protoline
// Set proto type picture line indexes
void
VS23S0x0::SetLineIndex (uint16_t line, uint16_t wordAddress)
{
  uint32_t indexAddr = INDEX_START_BYTES + line * 3;

  SpiRamWriteByte(indexAddr++, 0); // Byteaddress and bits to 0,
  // proto to 0
  SpiRamWriteByte(indexAddr++, wordAddress); // Actually it's
  // wordaddress
  SpiRamWriteByte(indexAddr, wordAddress >> 8);
}

void
VS23S0x0::setColorSpace (uint8_t palette)
{
  // 8. Set microcode program for picture lines
  // Use HROP1/HROP2/OP4/OP4 for 2 PLL clocks per pixel modes
  const uint8_t *ops = m_pal ? vs23_ops_pal[palette] : vs23_ops_ntsc[palette];
  SpiRamWriteProgram (PROGRAM,
		      (ops[3] << 8) | ops[2], (ops[1] << 8) | ops[0]);
  // Set color burst
  uint32_t w = PROTOLINE_WORD_ADDRESS(0) + BURST;
  for (int i = 0; i < BURSTDUR; i++)
    SpiRamWriteWord ((uint16_t)w++, BURST_LEVEL | (ops[4]) << 8);
}

// / Set picture type line indexes
void
VS23S0x0::SetPicIndex (uint16_t line,
		       uint32_t byteAddress,
		       uint16_t protoAddress)
{
  uint32_t indexAddr = INDEX_START_BYTES + line * 3;

  // Byteaddress LSB, bits to 0, proto to given value
  SpiRamWriteByte (indexAddr++,
		   (uint16_t)((byteAddress << 7) & 0x80)
		   | (protoAddress & 0xf));
  // This is wordaddress
  SpiRamWriteByte (indexAddr++, (uint16_t)(byteAddress >> 1));
  SpiRamWriteByte (indexAddr, (uint16_t)(byteAddress >> 9));
}

// Set picture pixel to a RGB value.
void
VS23S0x0::setPixelRgb (uint16_t xpos, uint16_t ypos, uint8_t r, uint8_t g,
		       uint8_t b)
{
  uint8_t pixdata = colorFromRgb(r, g, b);

#if 0
  {
    uint16_t wordaddress;

    wordaddress = PICLINE_WORD_ADDRESS(ypos) + xpos;
    SpiRamWriteWord(wordaddress, pixdata);
  }
#else
  {
    uint32_t byteaddress;

    byteaddress = pixelAddr(xpos, ypos);
    SpiRamWriteByte(byteaddress, pixdata);
  }
#endif
}

void
VS23S0x0::setPixelYuv (uint16_t xpos, uint16_t ypos, uint8_t color)
{
  uint32_t byteaddress = pixelAddr (xpos, ypos);
  SpiRamWriteByte(byteaddress, color);
}

// ---------------------------------------------------------------------------
// Equivalent to Config
// Initialize the VS32S0x0 chip
//
void
VS23S0x0::videoInit (uint8_t channel)
{
  uint16_t i, j;
  uint32_t w;

#ifdef DEBUG
  uint16_t linelen = PLLCLKS_PER_LINE;
  Serial.printf("Linelen: %d PLL clks\n", linelen);
  printf("Picture line area is %d x %d\n", PICX, PICY);
  printf("Upper left corner is point (0,0) and lower right corner (%d,%d)\n",
	 PICX - 1, PICY - 1);
  printf("Memory space available for picture bytes %ld\n",
	 131072 - PICLINE_START);
  printf("Free bytes %ld\n",
	 131072 - PICLINE_START - (PICX + BEXTRA) * PICY * PICBITS / 8);
  printf("Picture line %d bytes\n", PICLINE_LENGTH_BYTES + BEXTRA);
  printf("Picture length, %d color cycles\n", PICLENGTH);
  printf("Picture pixel bits %d\n", PICBITS);
  printf("Start pixel %x\n", (STARTPIX - 1));
  printf("End pixel %x\n", (ENDPIX - 1));
  printf("Index start address %x\n", INDEX_START_BYTES);
  printf("Picture line 0 address %lx\n", piclineByteAddress(0));
  printf("Last line %d\n", PICLINE_MAX);
#endif

  // Disable video generation
  SpiRamWriteRegister(VDCTRL2, 0);

  // 1. Select the first VS23 for following commands in case there
  // are several VS23 ICs connected to same SPI bus.
  SpiRamWriteByteRegister(WRITE_MULTIIC, 0xe);
  // 2. Set SPI memory address autoincrement
  SpiRamWriteByteRegister(WRITE_STATUS, 0x40);

  // 4. Write picture start and end
  SpiRamWriteRegister(PICSTART, (STARTPIX - 1));
  SpiRamWriteRegister(PICEND, (ENDPIX - 1));

  // 5. Enable PLL clock
  SpiRamWriteRegister(VDCTRL1,
		      (VDCTRL1_PLL_ENABLE) | (VDCTRL1_SELECT_PLL_CLOCK));
  // 6. Clear the video memory
  for (w = 0; w < 65536; w++)
    SpiRamWriteWord((uint16_t)w, 0x0000);	// Clear memory
  // 7. Set length of one complete line (unit: PLL clocks)
  SpiRamWriteRegister(LINELEN, (PLLCLKS_PER_LINE));
  // 9. Define where Line Indexes are stored in memory
  SpiRamWriteRegister(INDEXSTART, INDEX_START_LONGWORDS);
  // 10. Enable the PAL Y lowpass filter, not used in NTSC
  // SpiRamWriteBMCtrl(BLOCKMVC1,0,0,BLOCKMVC1_PYF);
  // 11. Set all line indexes to point to protoline 0 (which by
  // definition is in the beginning of the SRAM)
  for (i = 0; i < TOTAL_LINES; i++) {
    SetLineIndex(i, PROTOLINE_WORD_ADDRESS(0));
  }

  // At this time, the chip would continuously output the proto line 0.
  // This protoline will become our most "normal" horizontal line.
  // For TV-Out, fill the line with black level, and insert a few
  // pixels of sync level (0) and color burst to the beginning.
  // Note that the chip hardware adds black level to all nonproto
  // areas so protolines and normal picture have different meaning for
  // the same Y value.
  // In protolines, Y=0 is at sync level and in normal picture Y=0 is at
  // black level (offset +102).

  // In protolines, each pixel is 8 PLLCLKs, which in TV-out modes
  // means one color subcarrier cycle.  Each pixel has 16 bits (one
  // word):
  // VVVVUUUUYYYYYYYY.

  if (m_interlace) {
    uint16_t wi;
    // Construct protoline 0 and 1. Protoline 0 is used for most of the
    // picture.  Protoline 1 has a similar first half than 0, but the
    // end has a short sync pulse.  This is used for line 623.
    // Protoline 2 has a normal sync and color burst and nothing else.
    // It is used between vertical sync lines and visible lines, but is
    // not mandatory always.
    for (j = 0; j <= 2; j++) {
      wi = PROTOLINE_WORD_ADDRESS(j);
      // Set all to blank level.
      for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
	SpiRamWriteWord(wi++, BLANK_LEVEL);
      }
      // Set the color level to black
      wi = PROTOLINE_WORD_ADDRESS(j) + BLANKEND;
      for (i = BLANKEND; i < FRPORCH; i++) {
	SpiRamWriteWord(wi++, BLACK_LEVEL);
      }
      // Set HSYNC
      wi = PROTOLINE_WORD_ADDRESS(j);
      for (i = 0; i < SYNC; i++)
	SpiRamWriteWord(wi++, SYNC_LEVEL);
      // Set color burst
      wi = PROTOLINE_WORD_ADDRESS(j) + BURST;
      for (i = 0; i < BURSTDUR; i++)
	SpiRamWriteWord(wi++, BURST_LEVEL);

#if 0
      // For testing purposes, make some interesting pattern to
      // protos 0 and 1.
      // Protoline 2 is blank from color burst to line end.
      if (j < 2) {
	wi = PROTOLINE_WORD_ADDRESS(j) + BLANKEND + 55;
	for (i = BLANKEND + 55; i < FRPORCH; i++) {
	  SpiRamWriteWord(wi++, BLACK_LEVEL + 0x20);
	}
	wi = PROTOLINE_WORD_ADDRESS(j) + BLANKEND;
	SpiRamWriteWord(wi++, 0x7F);
	for (i = 1; i <= 50; i++) {
	  // "Proto-maximum" green level + color
	  // carrier wave
	  SpiRamWriteWord(wi++, 0x797F + i * 0x1300);
	}
	// To make red+blue strip
#define RED_BIT 0x0400
#define BLUE_BIT 0x0800
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 126,
			(0x7300 | BLACK_LEVEL) + RED_BIT);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 130,
			(0x7300 | BLACK_LEVEL) + RED_BIT +
			BLUE_BIT);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 134,
			(0x7300 | BLACK_LEVEL) + BLUE_BIT);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 138,
			(0x7300 | BLACK_LEVEL));

	// Max V and min U levels
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 146,
			0x79c1);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 147,
			0x79c1);
	SpiRamWriteWord(PROTOLINE_WORD_ADDRESS(j) + 148,
			0x79c1);

	// Orangish color to the end of proto line
	w = PROTOLINE_WORD_ADDRESS(0) + FRPORCH - 1;
	for (i = 0; i <= 90; i++) {
	  SpiRamWriteWord((uint16_t)w--,
			  (WHITE_LEVEL - 0x30) | 0xc400);
	}
      }
#endif
    }

    // Add to the second half of protoline 1 a short sync
    wi = PROTOLINE_WORD_ADDRESS(1) + COLORCLKS_LINE_HALF;
    for (i = 0; i < SHORTSYNCM; i++)
      SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
    // middle of line
    for (i = COLORCLKS_LINE_HALF + SHORTSYNCM; i < COLORCLKS_PER_LINE; i++) {
      SpiRamWriteWord(wi++, BLANK_LEVEL);	// To the end of the
      // line to blank level
    }

    // Now let's construct protoline 3, this will become our short+short
    // VSYNC line
    wi = PROTOLINE_WORD_ADDRESS(3);
    for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
      SpiRamWriteWord(wi++, BLANK_LEVEL);
    }
    wi = PROTOLINE_WORD_ADDRESS(3);
    for (i = 0; i < SHORTSYNC; i++)
      SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
    // beginning of line
    wi = PROTOLINE_WORD_ADDRESS(3) + COLORCLKS_LINE_HALF;
    for (i = 0; i < SHORTSYNCM; i++)
      SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
    // middle of line

    // Now let's construct protoline 4, this will become our long+long
    // VSYNC line
    wi = PROTOLINE_WORD_ADDRESS(4);
    for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
      SpiRamWriteWord(wi++, BLANK_LEVEL);
    }
    wi = PROTOLINE_WORD_ADDRESS(4);
    for (i = 0; i < LONGSYNC; i++)
      SpiRamWriteWord(wi++, SYNC_LEVEL);	// Long sync at the
    // beginning of line
    wi = PROTOLINE_WORD_ADDRESS(4) + COLORCLKS_LINE_HALF;
    for (i = 0; i < LONGSYNCM; i++)
      SpiRamWriteWord(wi++, SYNC_LEVEL);	// Long sync at the
    // middle of line

    // Now let's construct protoline 5, this will become our long+short
    // VSYNC line
    wi = PROTOLINE_WORD_ADDRESS(5);
    for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
      SpiRamWriteWord(wi++, BLANK_LEVEL);
    }
    wi = PROTOLINE_WORD_ADDRESS(5);
    for (i = 0; i < LONGSYNC; i++)
      SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
    // beginning of line
    wi = PROTOLINE_WORD_ADDRESS(5) + COLORCLKS_LINE_HALF;
    for (i = 0; i < SHORTSYNCM; i++)
      SpiRamWriteWord(wi++, SYNC_LEVEL);	// Long sync at the
    // middle of line

    // And yet a short+long sync line, protoline 6
    wi = PROTOLINE_WORD_ADDRESS(6);
    for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
      SpiRamWriteWord(wi++, BLANK_LEVEL);
    }
    wi = PROTOLINE_WORD_ADDRESS(6);
    for (i = 0; i < SHORTSYNC; i++)
      SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
    // beginning of line
    wi = PROTOLINE_WORD_ADDRESS(6) + COLORCLKS_LINE_HALF;
    for (i = 0; i < LONGSYNCM; i++)
      SpiRamWriteWord(wi++, SYNC_LEVEL);	// Long sync at the
    // middle of line

    // Just short sync line, the last one, protoline 7
    wi = PROTOLINE_WORD_ADDRESS(7);
    for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
      SpiRamWriteWord(wi++, BLANK_LEVEL);
    }
    wi = PROTOLINE_WORD_ADDRESS(7);
    for (i = 0; i < SHORTSYNC; i++)
      SpiRamWriteWord(wi++, SYNC_LEVEL);	// Short sync at the
    // beginning of line

  } else {	// interlace

		// Protolines for progressive NTSC, here is not created a protoline
		// corresponding to interlace protoline 2.
		// Construct protoline 0
    w = PROTOLINE_WORD_ADDRESS(0);	// Could be w=0 because proto 0 always
    // starts at address 0
    for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
      SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
    }

    // Set the color level to black
    //FIXME setBorder(0, 0);

    // Set HSYNC
    w = PROTOLINE_WORD_ADDRESS(0);
    for (i = 0; i < SYNC; i++)
      SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
    // Makes a black&white picture
    // for (i=0; i<BURSTDUR; i++) SpiRamWriteWord(w++,BLANK_LEVEL);

    // Now let's construct protoline 1, this will become our short+short
    // VSYNC line
    w = PROTOLINE_WORD_ADDRESS(1);
    for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
      SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
    }
    w = PROTOLINE_WORD_ADDRESS(1);
    for (i = 0; i < SHORTSYNC; i++) {
      // Short sync at the beginning of line
      SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
    }
    w = PROTOLINE_WORD_ADDRESS(1) + COLORCLKS_LINE_HALF;
    for (i = 0; i < SHORTSYNCM; i++) {
      // Short sync at the middle of line
      SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
    }

    // Now let's construct protoline 2, this will become our long+long
    // VSYNC line
    w = PROTOLINE_WORD_ADDRESS(2);
    for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
      SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
    }
    w = PROTOLINE_WORD_ADDRESS(2);
    for (i = 0; i < LONGSYNC; i++) {
      // Long sync at the beginning of line
      SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
    }
    w = PROTOLINE_WORD_ADDRESS(2) + COLORCLKS_LINE_HALF;
    for (i = 0; i < LONGSYNCM; i++) {
      // Long sync at the middle of line
      SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
    }

    // Now let's construct protoline 3, this will become our long+short
    // VSYNC line
    w = PROTOLINE_WORD_ADDRESS(3);
    for (i = 0; i <= COLORCLKS_PER_LINE; i++) {
      SpiRamWriteWord((uint16_t)w++, BLANK_LEVEL);
    }
    w = PROTOLINE_WORD_ADDRESS(3);
    for (i = 0; i < LONGSYNC; i++) {
      // Short sync at the beginning of line
      SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
    }
    w = PROTOLINE_WORD_ADDRESS(3) + COLORCLKS_LINE_HALF;
    for (i = 0; i < SHORTSYNCM; i++) {
      // Long sync at the middle of line
      SpiRamWriteWord((uint16_t)w++, SYNC_LEVEL);
    }
  }

  setColorSpace(1);

  // 12. Now set first eight lines of frame to point to NTSC sync lines
  // Here the frame starts, lines 1 and 2
  if (m_interlace) {
    if (m_pal) {
      for (i=0; i<2; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(4));
      }
      // Line 3
      SetLineIndex(2, PROTOLINE_WORD_ADDRESS(5));
      // Lines 4 and 5
      for (i=3; i<5; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
      }
      // At this point, lines 0 to 4 are VSYNC lines and all other lines are proto0.

      // Another batch of VSYNC lines is made between the two fields of interlaced PAL picture.
      // The lines start from 310.
      for (i=FIELD1START; i<FIELD1START+2; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
      }
      SetLineIndex(FIELD1START+2, PROTOLINE_WORD_ADDRESS(6));
      for (i=FIELD1START+3; i<FIELD1START+5; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(4));
      }
      for (i=FIELD1START+5; i<FIELD1START+7; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
      }
      SetLineIndex(FIELD1START+7, PROTOLINE_WORD_ADDRESS(7));

      // Set empty lines with sync and color burst to beginning of both frames.
      for (i=5; i<FRONT_PORCH_LINES; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
      }
      for (i=FIELD1START+8; i<FIELD1START+FRONT_PORCH_LINES+2; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
      }

      // These are the three last lines of the frame, lines 623-625
      SetLineIndex(TOTAL_LINES-1-3, PROTOLINE_WORD_ADDRESS(1));
      for (i=TOTAL_LINES-1-2; i<=TOTAL_LINES-1; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
      }
    } else {
      for (i = 0; i < 3; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
      }
      // Lines 4 to 6
      for (i = 3; i < 6; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(4));
      }
      // Lines 7 to 9
      for (i = 6; i < 9; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
      }
      // At this point, lines 0 to 8 are VSYNC lines and all other lines are
      // proto0.

      // Another batch of VSYNC lines is made between the two fields of
      // interlaced NTSC picture.
      // The lines start from 261.
      SetLineIndex(FIELD1START, PROTOLINE_WORD_ADDRESS(1));
      for (i = FIELD1START + 1; i < FIELD1START + 3; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
      }
      SetLineIndex(FIELD1START + 3, PROTOLINE_WORD_ADDRESS(6));
      for (i = FIELD1START + 4; i < FIELD1START + 6; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(4));
      }
      SetLineIndex(FIELD1START + 6, PROTOLINE_WORD_ADDRESS(5));
      for (i = FIELD1START + 7; i < FIELD1START + 9; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(3));
      }
      SetLineIndex(FIELD1START + 9, PROTOLINE_WORD_ADDRESS(7));

      // Set empty lines with sync and color burst to beginning of both
      // frames.
      for (i = 9; i < FRONT_PORCH_LINES; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
      }
      for (i = FIELD1START + 10; i < FIELD1START + FRONT_PORCH_LINES; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
      }
    }
  } else {	// interlace
    if (m_pal) {
      // For progressive PAL case, this is quite a minimum case.
      // Here the frame starts, lines 1 and 2
      for (i=0; i<2; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
      }
      // Line 3
      SetLineIndex(2, PROTOLINE_WORD_ADDRESS(3));
      // Lines 4 and 5
      for (i=3; i<5; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(1));
      }
      // These are three last lines of the frame, lines 310-312
      for (i=TOTAL_LINES-3; i<TOTAL_LINES; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(1));
      }
    } else {
      // For progressive NTSC case, this is quite a minimum case.
      // Here the frame starts, lines 1 to 3
      for (i = 0; i < 3; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(1));
      }
      // Lines 4 to 6
      for (i = 3; i < 6; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(2));
      }
      // Lines 7 to 9
      for (i = 6; i < 9; i++) {
	SetLineIndex(i, PROTOLINE_WORD_ADDRESS(1));
      }
    }
  }

  // 13. Set pic line indexes to point to protoline 0 and their
  // individual picture line.
  for (i = 0; i < ENDLINE - STARTLINE; i++) {
    SetPicIndex(i + STARTLINE, piclineByteAddress(i), 0);
    // All lines use picture line 0
    // SetPicIndex(i+STARTLINE, PICLINE_BYTE_ADDRESS(0),0);
    if (m_interlace) {
      // In interlaced case in both fields the same area is picture
      // box area.
      // XXX: In PAL example, it says "TOTAL_LINES/2" instead of FIELD1START
      SetPicIndex (i + STARTLINE + FIELD1START, piclineByteAddress(i),  0);
    }
  }

  // 14. Set number of lines, length of pixel and enable video
  // generation
  // The line count is TOTAL_LINES - 1 in the example, but the only
  // value that doesn't produce a crap picture is 263 (even interlaced,
  // PAL: 314), on both CRT and LCD displays, so we hard-code it here.

  // XXX: PAL on Daewoo LCD TV, screen mode 1: 314 has no color noise,
  //	but shifts one pixel up or down, depending on screen content;
  //	"standard" 313 has color noise, but remains in place.
  //	Does not seem to happen in other modes, may be a TV quirk.

  int total_lines;
  if (m_pal)
    total_lines = 314;
  else
    total_lines = 263;
  if (m_interlace)
    total_lines *= 2;

  SpiRamWriteRegister (VDCTRL2,
		       (VDCTRL2_LINECOUNT * (total_lines + m_line_adjust))
		       | (VDCTRL2_PIXEL_WIDTH * (PLLCLKS_PER_PIXEL - 1))
		       | (m_pal ? VDCTRL2_PAL : 0)
		       | (VDCTRL2_ENABLE_VIDEO));
}

void
VS23S0x0::begin (bool interlace, bool lowpass, uint8_t system)
{
  m_vsync_enabled = false;
  m_interlace = interlace;
  m_lowpass = lowpass;

  m_pal = system != 0;

  m_gpio_state = 0xf;
  SpiRamWriteRegister (WRITE_GPIO_CTRL, m_gpio_state);

  m_line_adjust = 0;

  setMode(0);
}

void VS23S0x0::reset()
{
  setColorSpace(1);
}

void VS23S0x0::setSyncLine (uint16_t line)
{
  if (line == 0) {
    m_vsync_enabled = false;
  } else {
    m_sync_line = line;
    m_vsync_enabled = true;
  }
}

uint16_t
VS23S0x0::currentLine()
{
  uint16_t cl = SpiRamReadRegister(CURLINE) & 0xfff;
  if (m_interlace && cl >= 262)
    cl -= 262;
  return cl;
}

static uint32_t
init_timer0 (void)
{
//  aux_reg_write (ARC_V2_TMR1_LIMIT, -1);
//  aux_reg_write (ARC_V2_TMR1_CONTROL, 2);
//  aux_reg_write (ARC_V2_TMR1_COUNT, 0);
  return 0;
}

void VS23S0x0::calibrateVsync()
{
  uint32_t now, now2, cycles;
  //while (currentLine() != 100) {};
  now = init_timer0 ();
  //while (currentLine() == 100) {};
  //while (currentLine() != 100) {};
  for (;;) {
    now2 = 0; // aux_reg_read (ARC_V2_TMR1_COUNT);
    cycles = now2 - now;
    if (abs((int)m_cycles_per_frame - (int)cycles) < 80000)
      break;
    m_cycles_per_frame = cycles;
    now = now2;
    //while (currentLine() == 100) {};
    //while (currentLine() != 100) {};
  }
  m_cycles_per_frame = cycles;
}

bool
VS23S0x0::setMode (uint8_t mode)
{
  setSyncLine(0);

  m_current_mode = m_pal ? &modes_pal[mode] : &modes_ntsc[mode];
  m_first_line_addr = PICLINE_BYTE_ADDRESS(0);
  m_pitch = PICLINE_BYTE_ADDRESS(1) - m_first_line_addr;

  videoInit(0);
  calibrateVsync();

#if 0
  if (m_pal && F_CPU / m_cycles_per_frame < 45) {
    // We are in PAL mode, but the hardware has an NTSC crystal.
    m_pal = false;
    goto retry;
  } else if (!m_pal && F_CPU / m_cycles_per_frame > 70) {
    // We are in NTSC mode, but the hardware has a PAL crystal.
    m_pal = true;
    goto retry;
  }
#endif

  // Start the new frame at the end of the visible screen plus a little extra.
  // Used to be two-thirds down the screen, but that caused more flicker when
  // the rendering load changes drastically.
  setSyncLine(m_current_mode->y + m_current_mode->top + 16);

  // Sony KX-14CP1 and possibly other displays freak out if we start drawing
  // stuff before they had a chance to synchronize with the new mode, so we
  // wait a few frames.
  delay(160);

  return true;
}
