#include <Arduino.h>
#include <SPI.h>
#include "vs23s0x0.h"

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
const struct tms9918_t {
  uint8_t r, g, b;
} tms9918[] =
  {
    {0x00, 0x00, 0x00},
    {0x0A, 0xAD, 0x1E},
    {0x34, 0xC8, 0x4C},
    {0x2B, 0x2D, 0xE3},
    {0x51, 0x4B, 0xFB},
    {0xBD, 0x29, 0x25},
    {0x1E, 0xE2, 0xEF},
    {0xFB, 0x2C, 0x2B},
    {0xFF, 0x5F, 0x4C},
    {0xBD, 0xA2, 0x2B},
    {0xD7, 0xB4, 0x54},
    {0x0A, 0x8C, 0x18},
    {0xAF, 0x32, 0x9A},
    {0xB2, 0xB2, 0xB2},
    {0xFF, 0xFF, 0xFF}
  };

enum colorid
  {
    black = 0,
    medium_green,
    light_green,
    dark_blue,
    light_blue,
    dark_red,
    cyan,
    medium_red,
    light_red,
    dark_yellow,
    light_yellow,
    dark_green,
    magenta,
    gray,
    white,
  };

//uint8_t plasma[256][200];

VS23S0x0 vs23;
static const byte CH0	= 0x00;
static const byte CH1	= 0x01;
static const byte CH2	= 0x02;
static const byte CH3	= 0x03;

static void Mandelbrot (uint8_t channel, float Xn, float Xp, float Yn, float Yp)
{
  float x0, y0, xtemp;
  float x = 0;
  float y = 0;
  uint16_t Px, Py;
  uint16_t iteration = 0;
  uint16_t max_iteration = 256;

  for (Py = 0; Py < vs23.height(); Py++)
    {
      y0 = (Yp - Yn)/vs23.height() * Py + Yn;
      for (Px = 0; Px < vs23.width(); Px++)
	{
	  x0 = (Xp - Xn)/vs23.width() * Px + Xn;
	  x = 0;
	  y = 0;
	  iteration = 0;
	  while ((x*x + y*y <= 2*2) && (iteration < max_iteration) )
	    {
	      xtemp = x*x - y*y + x0;
	      y = 2*x*y + y0;
	      x = xtemp;
	      iteration++;
	    }
	  vs23.setPixelYuv (Px, Py, (iteration & 0x0f)+ 0x20);
	}
    }
}


void setup() {

  // Wait for serial interface to come up.
  while (!Serial) ;
  Serial.begin (9600);
  Serial.println ("");
  Serial.println (F("P42 VGA Shield Test - Composite Output"));

  // Config pins
  pinMode (nWP_PIN, OUTPUT);
  digitalWrite (nWP_PIN, HIGH);
  pinMode (nHOLD_PIN, OUTPUT);
  digitalWrite (nHOLD_PIN, HIGH);

  pinMode(VS23_MVBLK_PIN, INPUT);

  // Config SPI interface
  pinMode (VS23_CS_PIN, OUTPUT);
  VS23_DESELECT; //digitalWrite (VS23_CS_PIN, HIGH);
  pinMode (MEMF_CS_PIN, OUTPUT);
  digitalWrite (MEMF_CS_PIN, HIGH);

  SPI.begin();
  SPI.setClockDivider (SPI_CLOCK_DIV2); //MAX speed -> 2
  SPI.setDataMode (SPI_MODE0);
  SPI.setBitOrder (MSBFIRST) ;

  // Setup VS23S0x0 chip
  vs23.begin(false, true, 1);

  Serial.println (F("Configuration done."));
}

// the loop function runs over and over again forever
void loop() {
#ifdef DEBUG
  // Draw some color bars
  {
    uint16_t re = 0;
    uint16_t gr = 0;
    uint16_t bl = 0;
    uint16_t xp = 0;

    for (i = 0; i < 256; i++) {

      for (j = 0; j < 16; j++) {
	xp = 10 + (i % 16) * 18 + j;
	if (xp > vs23.width() - 10)
	  continue;
	drawLine(xp, 6 + (i / 16) * 12, xp,
		 16 + (i / 16) * 12, i);
      }
    }

    // Frame around the picture box
    re = 0;
    gr = 255;
    bl = 0;
    drawLineRgb(0, 0, 0, PICY - 1, re, gr, bl);
    drawLineRgb(0, 0, PICX - 1, 0, re, gr, bl);
    drawLineRgb(PICX - 1, 0, PICX - 1, PICY - 1, re, gr, bl);
    drawLineRgb(0, PICY - 1, PICX - 1, PICY - 1, re, gr, bl);
  }
#endif

  uint32_t start_time = 0;
  uint32_t current_time = 0;
  int i,j;

  start_time = millis();

  Mandelbrot (CH0, -2.5, 1, -1, 1);

  current_time = millis();

  Serial.print (F("Mandelbrot duration [msec]"));
  Serial.println (current_time - start_time);

  Serial.println(F("4 RGB Colour Bars [press key]"));
  while (Serial.available() == 0) {};
  Serial.read();

  vs23.clearScreen (7);

#define XSIZEREC vs23.width()/16
#define YSIZEREC vs23.height()/16

  Serial.println("Draw colour map test image");
  uint8_t cc = 0;
  for (j=0; j<16; j++)
    for (i=0; i<16; i++)
      {
	vs23.fillRectangle ((i*XSIZEREC), (j*YSIZEREC),
			    (i*XSIZEREC)+(XSIZEREC-1),
			    (j*YSIZEREC)+(YSIZEREC-1), cc++);
      }
  Serial.println(F("Display Image [press key]") );
  while (Serial.available() == 0) {};
  Serial.read();

  start_time = millis();

  uint16_t x = 0;
  uint16_t y = 0;
  for (i = 0; i < vs23.height(); i++)
    for (j = x; j < vs23.width()/8; j++)
      vs23.setPixelRgb (j, i, 0xff, 0x00, 0x00);

  x = vs23.width()/8;
  y = 0;
  for (i = 0; i < vs23.height(); i++)
    for (j = x; j < 2 * vs23.width()/8; j++)
      vs23.setPixelRgb (j, i, 0x00, 0xff, 0x00);

  x = 2 * vs23.width()/8;
  y = 0;
  for (i = 0; i < vs23.height(); i++)
    for (j = x; j < 3 * vs23.width()/8; j++)
      vs23.setPixelRgb (j, i, 0x00, 0x00, 0xff);

  current_time = millis();

  for (int k = 0; k < 15; k++)
    {
      x = 3 * vs23.width()/8;
      y = 0;
      for (i = 0; i < vs23.height(); i++)
  	for (j = x; j < 4 * vs23.width()/8; j++)
  	  vs23.setPixelRgb (j, i,
  			    tms9918[k].r,
  			    tms9918[k].g,
  			    tms9918[k].b);
      while (Serial.available() == 0) {};
      Serial.read();
    }


  Serial.print (F("RGB bars duration [msec]"));
  Serial.println (current_time - start_time);

  // more colour bars
  Serial.println(F("4 YUV Pixel [press key]") );

  x = 4 * vs23.width()/8;
  vs23.fillRectangle (x, 0, (5*vs23.width()+8)/8-1, vs23.height(), 0x24);

  x = 5 * vs23.width()/8;
  vs23.fillRectangle (x, 0, (6*vs23.width()+8)/8-1, vs23.height(), 0x94);

  x = 6 * vs23.width()/8;
  vs23.fillRectangle (x, 0, (7*vs23.width()+8)/8-1, vs23.height(), 0xbf);

  x = 7 * vs23.width()/8;
  vs23.fillRectangle (x, 0, vs23.width(), vs23.height(), 83);

  delay(10);
  //Plasma
  #if 0
  for (x = 0; x < vs23.width(); x++)
    for (y = 0; y < vs23.height(); y++)
      {
	float fcolor = 128.0 + (128.0 * sin (x / 16.0)) +
	  128.0 + (128.0 * sin (y / 16.0));
	uint8_t color = (uint8_t) fcolor >> 1;
	plasma[x][y] = color;
      }
#endif
  while (Serial.available() == 0)
    {
      uint8_t palleteShift = random (0, 255);
      for (y = 0; y < vs23.height(); y++)
	{
	  for (x = 0; x < vs23.width(); x++)
	    {
	      float fcolor = 128.0 + (128.0 * sin (x / 16.0)) +
		128.0 + (128.0 * sin (y / 16.0));
	      uint8_t color = (uint8_t) fcolor + palleteShift;
	      vs23.setPixelRgb (x, y,
				color, color*2, 255-color);
	    }
	}
    }
  Serial.read();
  Serial.println(F("End of test! [Restart press key]"));
  delay(1);
  while (Serial.available() == 0) {};
  Serial.read();
}
