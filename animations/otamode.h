#ifndef OTAMODE_H
#define OTAMODE_H

#include "animation.h"

#define OTAMODE_COLOR_ONE CRGB(0, 0, 0)
#define OTAMODE_COLOR_TWO CRGB(255, 0, 0)

void OTAModeAnimation(const AnimationInputs& inputs) {
  uint8_t percent = cylon_signal(TO_MS(inputs.raw_time_us), 3, 255);

  Serial.print("Percent: ");
  Serial.println(percent);

  CRGB color = blend(OTAMODE_COLOR_ONE, OTAMODE_COLOR_TWO, percent);

  fill_solid(inputs.leds, inputs.num_leds, color);
}

#endif // OTAMODE_H
