#include "Arduino.h"
#include "tms9918.h"
#include "vs23s0x0-hal.h"
#include <SPI.h>

#include "font-8x8.h"
#include "ball.h"

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
  unsigned char *font = (unsigned char *)console_font_8x8;

    // Wait for serial interface to come up.
  while (!Serial) ;
  Serial.begin (9600);
  Serial.println ("");
  Serial.println (F("P42 VGA Shield Test - TMS9918a EMU"));

  videoConfigPins();
  tms9918aInit ();

  /* 1. Initialize Text Mode.  */
  tms9918aWriteReg (0, 0);    /* Text mode, no external video.  */
  tms9918aWriteReg (1, 0xc0); /* 16k, enable disp, disable int.  */
  tms9918aWriteReg (2, 0x02); /* Address of Name Table in VRAM = 0x800.  */
  tms9918aWriteReg (3, 0x30); /* Address of Color Table in VRAM = 0xC00.  */
  tms9918aWriteReg (4, 0);    /* Address of Pattern Table in VRAM = 0x000. */
  tms9918aWriteReg (7, 0xf5); /* White text on Light Blue Background.  */

  /* 2. initialize Pattern Table with the desired font.  */
  for (int i = 0; i < (256 * 8); i++)
    tms9918aWriteData (*font++);

  /* 3. Initialize color Table.  */
  tms9918aWriteAddr (0xc00);
  for (int i = 0; i < 32; i++)
    tms9918aWriteData (((i+2)%16) << 4 | 1);

  /* 3. Update Screen. */
  tms9918aDisplay ();
  Serial.println (F("Initialization completed"));
}

void loop () {
  const char message[] = "Hello World TMS9918a emulation.";
  char *pmsg;
  const uint32_t addrNameTable = 0x800;

  pmsg = (char *) message;
  tms9918aWriteAddr (addrNameTable);
  while (*pmsg != 0x00)
    {
      char chr = *pmsg++;
      tms9918aWriteData (chr);
    }
  tms9918aDisplay ();

  Serial.println(F("Next"));
  delay(1);
  while (Serial.available() == 0) {};
  Serial.read();

  pmsg = (char *) ball;
  tms9918aWriteAddr (addrNameTable);
  while (*pmsg != 0)
    {
      char chr = *pmsg++;
      tms9918aWriteData (chr);
    }
  tms9918aDisplay ();
  Serial.println(F("Next"));
  delay(1);
  while (Serial.available() == 0) {};
  Serial.read();

  tms9918aWriteAddr (0xc00);
  for (int i = 0; i < 32; i++)
    {
      uint8_t val = tms9918aVramValue (0xc00 + i);
      val = ((val >> 4) + 1) % 16;
      tms9918aWriteData ((val << 4) | 1);
    }
  tms9918aWriteReg (7, 0xf6); /* White text on Light Blue Background.  */
  tms9918aDisplay ();

  Serial.println(F("End of test! [Restart press key]"));
  delay(1);
  while (Serial.available() == 0) {};
  Serial.read();
}
