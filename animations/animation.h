#ifndef ANIMATION_H
#define ANIMATION_H

#include <FastLED.h>

class Animation {
  public:
    Animation(CRGB* leds, int num_leds, int num_frames)
             : leds_(leds), num_leds_(num_leds),
               num_frames_(num_frames), frame_(0) {};

    bool nextFrame() {
      generateNextFrame();
      frame_++;
      return (frame_ < num_frames_);
    };

    virtual void generateNextFrame() = 0;

  protected:

    CRGB randomColor() const {
      return CHSV(random(255), 255, 255);
    }

    void fillRandomContrastingColors(CRGB &c1, CRGB &c2) const {
      int h1 = random(255);
      int offset = random(180) - 90;
      int h2 = (h1 + 128 + offset) % 255;
      
      c1 = CHSV(h1, 255, 255);
      c2 = CHSV(h2, 255, 255);
    }

    int randomDirection() const {
      return random(0, 2) ? 1 : -1;
    }

    CRGB* leds_;
    int num_leds_, num_frames_, frame_;
};

#endif  // ANIMATION_H
