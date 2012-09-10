#include <math.h>
#include <stdint.h>
#include "color.h"

static const float ONE_THIRD = 1.0/3.0;
static const float ONE_SIXTH = 1.0/6.0;
static const float TWO_THIRD = 2.0/3.0;

static float _v(const float m1, const float m2, float hue) {
    hue = fmod(hue, 1.0);
    if (hue < ONE_SIXTH)
        return m1 + (m2 - m1) * hue * 6.0;
    if (hue < 0.5)
        return m2;
    if (hue < TWO_THIRD)
        return m1 + (m2 - m1) * (TWO_THIRD - hue) * 6.0;
    return m1;
}

static inline uint8_t float_to_uint8(float value) {
    return value * 255;
}

void hls_to_rgb(const float h, const float l, const float s, uint8_t *r, uint8_t *g, uint8_t *b) {
    float m1, m2;
    if (s <= 0.0) {
        *r = float_to_uint8(l);
        *g = float_to_uint8(l);
        *b = float_to_uint8(l);
        return;
    }
    if (l <= 0.5) {
        m2 = l * (1.0 + s);
    } else {
        m2 = l + s - (l * s);
    }
    m1 = 2.0 * l - m2;

    *r = float_to_uint8(_v(m1, m2, h+ONE_THIRD));
    *g = float_to_uint8(_v(m1, m2, h));
    *b = float_to_uint8(_v(m1, m2, h-ONE_THIRD));
    return;
}
