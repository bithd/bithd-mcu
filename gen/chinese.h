#ifndef _CHINESE_H_
#define _CHINESE_H_

#include <stdint.h>

#define ZHFONT_WIDTH	12
#define ZHASCII_WIDTH	8
#define ZHFONT_HEIGHT	12

typedef struct {
	uint8_t index[3];
	const uint8_t font12[24];
} ChineseMask;
/*
typedef struct {
	uint8_t index[3];
	const uint8_t ascii[12];
}AsciiMask;
*/
int ChineseMaskSize(void);
extern const ChineseMask zh_font[];

//extern const AsciiMask zh_ascii[];


#endif //_CHINESE_H_
