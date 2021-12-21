#include "Arduino.h"
#include "tms9918.h"
#include "vs23s0x0-hal.h"
#include <SPI.h>
#include "bird.h"
#include "mcmode.h"

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
    // Wait for serial interface to come up.
  while (!Serial) ;
  Serial.begin (9600);
  Serial.println ("");
  Serial.println (F("P42 VGA Shield Test - TMS9918a EMU"));

  videoConfigPins();
  tms9918aInit ();

  /* 1. Initialize MultiColor Mode.  */
  tms9918aWriteReg (0, 0);    /* Multicolor mode, no external video.  */
  tms9918aWriteReg (1, 0xcb); /* 16k, enable disp, disable int */
  tms9918aWriteReg (2, 0x05); /* Address of Name Table in VRAM = 0x1400.  */
  tms9918aWriteReg (4, 0x01); /* Address of Pattern Table in VRAM = 0x800. */
  tms9918aWriteReg (5, 0x20); /* Address of Sprite Attribute Table in VRAM = 0x1000. */
  tms9918aWriteReg (6, 0);    /* Address of Sprite Pattern Table in VRAM = 0x000. */
  tms9918aWriteReg (7, 0x04); /* Backdrop Color Dark Blue.  */

  /* 2. initialize Name Table.  */
  tms9918aWriteAddr ((tms9918aRegValue (2) & 0x0f) << 10);
  for (int y = 0; y <= 23; y++)
    {
      for (int x = 0; x <= 31; x++)
	{
	  uint8_t data = x + ((y & 0xfc) << 3);
	  tms9918aWriteData (data);
	}
    }

  /* 3. Update Screen. */
  tms9918aDisplay ();
  Serial.println (F("Initialization completed"));
}

void loop () {

  /* Set Address Pattern Table.  */
  tms9918aWriteAddr ((tms9918aRegValue (4) & 0x07) << 11);
  for (int i = 0; i < 0x800; i++)
    tms9918aWriteData (bird[i]);

  tms9918aDisplay ();
  Serial.println(F("Next"));
  delay(1);
  while (Serial.available() == 0) {};
  Serial.read();

  tms9918aWriteAddr ((tms9918aRegValue (4) & 0x07) << 11);
  for (int i = 0; i < 0x800; i++)
    tms9918aWriteData (mcmode[i]);

  tms9918aDisplay ();
  Serial.println(F("End of test! [Restart press key]"));
  delay(1);
  while (Serial.available() == 0) {};
  Serial.read();
}
