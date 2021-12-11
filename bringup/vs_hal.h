#ifndef __VS_HAL_H__
#define __VS_HAL_H__

#define VS23_CS_PIN 10
#define VS23_MVBLK_PIN 6

#define MEMF_CS_PIN 9
#define nHOLD_PIN 8
#define nWP_PIN 7


#define VS23_SELECT digitalWrite(VS23_CS_PIN, LOW)
#define VS23_DESELECT digitalWrite(VS23_CS_PIN, HIGH)
#define VS23_MBLOCK digitalRead(VS23_MVBLK_PIN)

//#ifndef LOW
//#define LOW 0
//#endif
//#ifndef HIGH
//#define HIGH 1
//#endif
//#ifndef OUTPUT
//#define OUTPUT 1
//#endif

//inline void digitalWrite1 (int a, int b)
//{
//  printf ("<DIG_W> P:%d, %s\n", a, b == 0 ? "Low": "High");
//}
//
//inline uint8_t digitalRead1 (int a)
//{
//  //printf ("<DIG_R> P:%d\n", a);
//  return LOW;
//}

#ifndef abs
#define abs(x) ((x)>0?(x):-(x))
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) < (b)) ? (b) : (a))
#endif

#ifndef delay
#define delay(x)
#endif

#endif
