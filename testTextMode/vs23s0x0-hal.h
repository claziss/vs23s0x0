#ifndef __VS23S0x0_HAL_H__
#define __VS23S0x0_HAL_H__

#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VS23_CS_PIN 10
#define VS23_MVBLK_PIN 6

#define MEMF_CS_PIN 9
#define nHOLD_PIN 8
#define nWP_PIN 7

#define VS23_SELECT digitalWrite(VS23_CS_PIN, LOW)
#define VS23_DESELECT digitalWrite(VS23_CS_PIN, HIGH)
#define VS23_MBLOCK digitalRead(VS23_MVBLK_PIN)

extern uint8_t spi_transfer (uint8_t);
extern uint16_t spi_transfer16 (uint16_t);
extern void spi_transfer24 (uint32_t);
extern void spi_transfer32 (uint32_t);

#ifdef __cplusplus
}
#endif

#endif
