#ifndef __VS23S0x0_INTERNAL_H__
#define __VS23S0x0_INTERNAL_H__

/// Crystal frequency in MHZ (float, observe accuracy)
#define XTAL_MHZ_NTSC 3.579545
#define XTAL_MHZ_PAL 4.43361875
#define XTAL_MHZ (m_pal ? XTAL_MHZ_PAL : XTAL_MHZ_NTSC)

/// Line length in microseconds (float, observe accuracy)
#define LINE_LENGTH_US_NTSC 63.5555
#define LINE_LENGTH_US_PAL 64.0
#define LINE_LENGTH_US (m_pal ? LINE_LENGTH_US_PAL : LINE_LENGTH_US_NTSC)

/// Frame length in lines (visible lines + nonvisible lines)
/// Amount has to be odd for NTSC and RGB colors
#define TOTAL_LINES_INTERLACE_NTSC 525
#define TOTAL_LINES_INTERLACE_PAL 625
#define TOTAL_LINES_INTERLACE (m_pal ? TOTAL_LINES_INTERLACE_PAL : TOTAL_LINES_INTERLACE_NTSC)

#define FIELD1START_NTSC 261
#define FIELD1START_PAL 310
#define FIELD1START (m_pal ? FIELD1START_PAL : FIELD1START_NTSC)

#define TOTAL_LINES_PROGRESSIVE_NTSC 262
#define TOTAL_LINES_PROGRESSIVE_PAL 313	// or 312?
#define TOTAL_LINES_PROGRESSIVE (m_pal ? TOTAL_LINES_PROGRESSIVE_PAL : TOTAL_LINES_PROGRESSIVE_NTSC)

#define TOTAL_LINES (m_interlace ? TOTAL_LINES_INTERLACE : TOTAL_LINES_PROGRESSIVE)

/// Number of lines used after the VSYNC but before visible area.
#define FRONT_PORCH_LINES_NTSC 20
#define FRONT_PORCH_LINES_PAL 22
#define FRONT_PORCH_LINES (m_pal ? FRONT_PORCH_LINES_PAL : FRONT_PORCH_LINES_NTSC)

/// Width, in PLL clocks, of each pixel
/// Used 4 to 8 for 160x120 pics
#define PLLCLKS_PER_PIXEL (m_current_mode->vclkpp)

/// Extra bytes can be added to end of picture lines to prevent pic-to-proto
/// border artifacts. 8 is a good value. 0 can be tried to test, if there is
/// no need for extra bytes.
#define BEXTRA (m_current_mode->bextra)

/// Definitions for picture lines
/// On which line the picture area begins, the Y direction.
//#define STARTLINE ((uint16_t)(TOTAL_LINES/4))
#define STARTLINE (FRONT_PORCH_LINES+m_current_mode->top)
/// The last picture area line
#define ENDLINE STARTLINE + YPIXELS
/// The first pixel of the picture area, the X direction.
#define STARTPIX (BLANKEND + m_current_mode->left)
/// The last pixel of the picture area. Set PIXELS to wanted value and suitable
/// ENDPIX value is calculated.
#define ENDPIX ((uint16_t)(STARTPIX + PLLCLKS_PER_PIXEL * XPIXELS/8))

/// Reserve memory for this number of different prototype lines
/// (prototype lines are used for sync timing, porch and border area)
#define PROTOLINES_INTERLACE 8
#define PROTOLINES_PROGRESSIVE 4
#define PROTOLINES (m_interlace ? PROTOLINES_INTERLACE : PROTOLINES_PROGRESSIVE)

/// Protoline lenght is the real lenght of protoline (optimal memory
/// layout, but visible lines' prototype must always be proto 0)
#define PROTOLINE_LENGTH_WORDS ((uint16_t)(LINE_LENGTH_US*XTAL_MHZ+0.5))

/// PLL frequency
#define PLL_MHZ (XTAL_MHZ * 8.0)
/// 10 first pllclks, which are not in the counters are decremented here
#define PLLCLKS_PER_LINE ((uint16_t)((LINE_LENGTH_US * PLL_MHZ)+0.5-10))
/// 10 first pllclks, which are not in the counters are decremented here
#define COLORCLKS_PER_LINE ((uint16_t)((LINE_LENGTH_US * XTAL_MHZ)+0.5-10.0/8.0))
#define COLORCLKS_LINE_HALF ((uint16_t)((LINE_LENGTH_US * XTAL_MHZ)/2+0.5-10.0/8.0))

#define PROTO_AREA_WORDS (PROTOLINE_LENGTH_WORDS * PROTOLINES)
#define INDEX_START_LONGWORDS ((PROTO_AREA_WORDS+1)/2)
#define INDEX_START_WORDS (INDEX_START_LONGWORDS * 2)
#define INDEX_START_BYTES (INDEX_START_WORDS * 2)

/// Define NTSC video timing constants
/// NTSC short sync duration is 2.35 us
#define SHORT_SYNC_US_NTSC 2.542
#define SHORT_SYNC_US_PAL 2.35
#define SHORT_SYNC_US (m_pal ? SHORT_SYNC_US_PAL : SHORT_SYNC_US_NTSC)

/// For the start of the line, the first 10 extra PLLCLK sync (0) cycles
/// are subtracted.
#define SHORTSYNC ((uint16_t)(SHORT_SYNC_US*XTAL_MHZ-10.0/8.0))
/// For the middle of the line the whole duration of sync pulse is used.
#define SHORTSYNCM ((uint16_t)(SHORT_SYNC_US*XTAL_MHZ))
/// NTSC long sync duration is 27.3 us
#define LONG_SYNC_US_NTSC 27.33275
#define LONG_SYNC_US_PAL 27.3
#define LONG_SYNC_US (m_pal ? LONG_SYNC_US_PAL : LONG_SYNC_US_NTSC)

#define LONGSYNC ((uint16_t)(LONG_SYNC_US*XTAL_MHZ))
#define LONGSYNCM ((uint16_t)(LONG_SYNC_US*XTAL_MHZ))
/// Normal visible picture line sync length is 4.7 us
#define SYNC_US 4.7
#define SYNC ((uint16_t)(SYNC_US*XTAL_MHZ-10.0/8.0))
/// Color burst starts at 5.6 us
#define BURST_US_NTSC 5.3
#define BURST_US_PAL 5.6
#define BURST_US (m_pal ? BURST_US_PAL : BURST_US_NTSC)

#define BURST ((uint16_t)(BURST_US*XTAL_MHZ-10.0/8.0))
/// Color burst duration is 2.25 us
#define BURST_DUR_US_NTSC 2.67
#define BURST_DUR_US_PAL 2.25
#define BURST_DUR_US (m_pal ? BURST_DUR_US_PAL : BURST_DUR_US_NTSC)

#define BURSTDUR ((uint16_t)(BURST_DUR_US*XTAL_MHZ))
/// NTSC sync to blanking end time is 10.5 us
#define BLANK_END_US_NTSC 9.155
#define BLANK_END_US_PAL 10.5
#define BLANK_END_US (m_pal ? BLANK_END_US_PAL : BLANK_END_US_NTSC)

#define BLANKEND ((uint16_t)(BLANK_END_US*XTAL_MHZ-10.0/8.0))
/// Front porch starts at the end of the line, at 62.5us
#define FRPORCH_US_NTSC 61.8105
#define FRPORCH_US_PAL 62.5
#define FRPORCH_US (m_pal ? FRPORCH_US_PAL : FRPORCH_US_NTSC)

#define FRPORCH ((uint16_t)(FRPORCH_US*XTAL_MHZ-10.0/8.0))

/// Select U, V and Y bit widths for 16-bit or 8-bit wide pixels.
#ifndef BYTEPIC
#define UBITS 4
#define VBITS 4
#define YBITS 8
#else
#define UBITS 2
#define VBITS 2
#define YBITS 4
#endif

/// Protoline 0 starts always at address 0
#define PROTOLINE_BYTE_ADDRESS(n) (PROTOLINE_LENGTH_WORDS*2*(n))
#define PROTOLINE_WORD_ADDRESS(n) (PROTOLINE_LENGTH_WORDS*(n))

/// Calculate picture lengths in pixels and bytes, coordinate areas for picture area
#define PICLENGTH (ENDPIX-STARTPIX)
#define PICX ((uint16_t)(PICLENGTH*8/PLLCLKS_PER_PIXEL))
#define PICY (ENDLINE-STARTLINE)
#define PICBITS (UBITS+VBITS+YBITS)
#ifndef BYTEPIC
#define PICLINE_LENGTH_BYTES ((uint16_t)(PICX*PICBITS/8+0.5))
#else
#define PICLINE_LENGTH_BYTES ((uint16_t)(PICX*PICBITS/8+0.5+1))
#endif
/// Picture area memory start point
#define PICLINE_START (INDEX_START_BYTES+TOTAL_LINES*3+1)
/// Picture area line start addresses
#define PICLINE_WORD_ADDRESS(n) (PICLINE_START/2+(PICLINE_LENGTH_BYTES/2+BEXTRA/2)*(n))
#define PICLINE_BYTE_ADDRESS(n) ((uint32_t)(PICLINE_START+((uint32_t)(PICLINE_LENGTH_BYTES)+BEXTRA)*(n)))

#define PICLINE_MAX ((131072-PICLINE_START)/(PICLINE_LENGTH_BYTES+BEXTRA))

/// 8-bit RGB to 8-bit YUV444 conversion
#define YRGB(r,g,b) ((76*r+150*g+29*b)>>8)
#define URGB(r,g,b) (((r<<7)-107*g-20*b)>>8)
#define VRGB(r,g,b) ((-43*r-84*g+(b<<7))>>8)

/// Pattern generator microcode
/// ---------------------------
/// Bits 7:6  a=00|b=01|y=10|-=11
/// Bits 5:3  n pick bits 1..8
/// bits 2:0  shift 0..6
#define PICK_A (0<<6)
#define PICK_B (1<<6)
#define PICK_Y (2<<6)
#define PICK_NOTHING (3<<6)
#define PICK_BITS(a)(((a)-1)<<3)
#define SHIFT_BITS(a)(a)

// For 2 PLL clocks per pixel modes:
#define HROP1 (PICK_Y + PICK_BITS(4) + SHIFT_BITS(4))
#define HROP2 (PICK_A + PICK_BITS(4) + SHIFT_BITS(4))

/// General VS23 commands
#define WRITE_STATUS 0x01
#define WRITE_MULTIIC 0xb8
#define WRITE_GPIO_CTRL 0x82
#define WRITE_SRAM 0x02

/// Bit definitions
#define VDCTRL1 0x2B
#define VDCTRL1_UVSKIP (1<<0)
#define VDCTRL1_DACDIV (1<<3)
#define VDCTRL1_PLL_ENABLE (1<<12)
#define VDCTRL1_SELECT_PLL_CLOCK (1<<13)
#define VDCTRL1_USE_UVTABLE (1<<14)
#define VDCTRL1_DIRECT_DAC (1<<15)

#define VDCTRL2 0x2D
#define VDCTRL2_LINECOUNT (1<<0)
#define VDCTRL2_PIXEL_WIDTH (1<<10)
#define VDCTRL2_NTSC (0<<14)
#define VDCTRL2_PAL (1<<14)
#define VDCTRL2_ENABLE_VIDEO (1<<15)

#define BLOCKMVC1_PYF (1<<4)

/// VS23 video commands
#define PROGRAM 0x30
#define PICSTART 0x28
#define PICEND 0x29
#define LINELEN 0x2a
#define LINELEN_VGP_OUTPUT (1<<15)
#define YUVBITS 0x2b
#define INDEXSTART 0x2c
#define LINECFG 0x2d
#define VTABLE 0x2e
#define UTABLE 0x2f
#define BLOCKMVC1 0x34
#define BLOCKMVC2 0x35
#define BLOCKMV_S 0x36
#define CURLINE 0x53

/// Sync, blank, burst and white level definitions, here are several options
/// These are for proto lines and so format is VVVVUUUUYYYYYYYY
/// Sync is always 0
#define SYNC_LEVEL  0x0000
/// one LSB is 5.1724137mV
#define SYNC_LEVEL  0x0000	// XXX: why is this here twice?
/// 285 mV to 75 ohm load
#define BLANK_LEVEL 0x0066
/// 339 mV to 75 ohm load
#define BLACK_LEVEL 0x0066
/// 285 mV burst
#define BURST_LEVEL 0x66	// add (burst vector << 8) to this
#define WHITE_LEVEL 0x00ff

#endif
