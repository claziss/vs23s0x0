#ifndef _VR_EMU_TMS9918A_CORE_H_
#define _VR_EMU_TMS9918A_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint8_t tms9918aRegValue (uint8_t);
uint8_t tms9918aVramValue (uint32_t);
uint8_t tms9918aReadData (void);
void tms9918aWriteAddr(uint16_t);
void tms9918aDisplay (void);
void tms9918aInit (void);

void tms9918aWriteData (uint8_t data);
void tms9918aWriteReg(uint8_t regno, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif
