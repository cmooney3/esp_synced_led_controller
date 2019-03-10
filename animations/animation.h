#ifndef ANIMATION_H
#define ANIMATION_H

#include <FastLED.h>

constexpr int ANIMATION_DURATION_MS = 6000;

constexpr int US_PER_MS = 1000;
constexpr int US_PER_S = 1000000;

#define TO_MS(x) (x / US_PER_MS)

// CRGB randomColor() {
//     return CHSV(random(255), 255, 255);
// }
// 
// void fillRandomContrastingColors(CRGB &c1, CRGB &c2) {
//     int h1 = random(255);
//     int offset = random(180) - 90;
//     int h2 = (h1 + 128 + offset) % 255;
//     
//     c1 = CHSV(h1, 255, 255);
//     c2 = CHSV(h2, 255, 255);
// }
// 
// int randomDirection() {
//     return random(0, 2) ? 1 : -1;
// }

#endif  // ANIMATION_H
