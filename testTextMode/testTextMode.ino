#include "Arduino.h"
#include "tms9918.h"
#include "font-6x8.h"
#include "vs23s0x0-hal.h"
#include <SPI.h>

uint8_t spi_transfer (uint8_t a)
{
  return SPI.transfer (a);
}
uint16_t spi_transfer16 (uint16_t a)
{
  return SPI.transfer16 (a);
}
void spi_transfer24 (uint32_t a)
{
  SPI.transfer24 (a);
}
void spi_transfer32 (uint32_t a)
{
  SPI.transfer32 (a);
}

static inline void videoConfigPins (void)
{
  // Config pins
  pinMode (nWP_PIN, OUTPUT);
  digitalWrite (nWP_PIN, HIGH);
  pinMode (nHOLD_PIN, OUTPUT);
  digitalWrite (nHOLD_PIN, HIGH);

  // Config SPI interface
  pinMode (VS23_CS_PIN, OUTPUT);
  VS23_DESELECT;
  pinMode (MEMF_CS_PIN, OUTPUT);
  digitalWrite (MEMF_CS_PIN, HIGH);

  SPI.begin();
  SPI.setClockDivider (SPI_CLOCK_DIV2); //MAX speed
  SPI.setDataMode (SPI_MODE0);
  SPI.setBitOrder (MSBFIRST);
}

void setup () {
  unsigned char *font = console_font_6x8;

  videoConfigPins();
  tms9918aInit ();

  /* 1. initialize Pattern Table with the desired font.  */
  for (int i = 0; i < (256 * 8); i++)
    tms9918aWriteData (*font++);

  /* 2. Initialize Text Mode.  */
  tms9918aWriteReg (0, 0);    /* Text mode, no external video.  */
  tms9918aWriteReg (1, 0xd0); /* 16k, enable disp, disable int.  */
  tms9918aWriteReg (2, 0x02); /* Address of Name Table in VRAM = 0x800.  */
  tms9918aWriteReg (4, 0);    /* Address of Pattern Table in VRAM = 0x000. */
  tms9918aWriteReg (7, 0xf5); /* White text on Light Blue Background.  */

  /* 3. Update Screen. */
  tms9918aDisplay ();
}

void loop () {
  const char message[] = "Hello World TMS9918a emulation.";
  char *pmsg;
  uint32_t addrNameTable = 0x800;

  pmsg = (char *) message;
  tms9918aWriteAddr (addrNameTable);
  while (pmsg != 0)
    {
      char chr = *pmsg++;
      tms9918aWriteData (chr);
    }
  tms9918aDisplay ();
}
