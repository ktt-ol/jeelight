#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>

void hsl_to_rgb(float h, float s, float l, uint8_t *r, uint8_t *g, uint8_t *b);

#endif