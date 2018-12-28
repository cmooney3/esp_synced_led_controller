#ifndef DROP_H
#define DROP_H

#include "animation.h"

namespace Drop {

constexpr int kMinBlockWidth = 1;
constexpr int kMaxBlockWidth = 1;

constexpr int kMinSpeed = 1;
constexpr int kMaxSpeed = 1;

// How many frames to pause after a full "fill" before switching colors
constexpr int kNumPauseFrames = 10;

class DropAnimation : public Animation {
  public:
    DropAnimation(CRGB* leds, int num_leds, int num_frames) :
        Animation(leds, num_leds, num_frames) {
      fillRandomContrastingColors(c1_, c2_);
      speed_ = random(kMinSpeed, kMaxSpeed);
      block_width_ = random(kMinBlockWidth, kMaxBlockWidth);
      fill_level_ = 0;
      dropping_block_position_ = 0;
      pause_timer_ = 0;
    }

    void generateNextFrame() {
      // If a pause timer has been set, decrement it and skip this frame.
      if (pause_timer_ > 0) {
        pause_timer_--;
        return;
      }

      // Draw in the filled area, then color in the currently dropping block.
      for (int i = 0; i < num_leds_; i++) {
        leds_[i] = (i > num_leds_ - fill_level_) ? c1_ : c2_;
      }
      for (int i = 0; i < block_width_; i++) {
        if (dropping_block_position_ + i >= 0) {
          leds_[dropping_block_position_ + i] = c1_;
        }
      }

      // If the whole thing is "full" swap the colors and reset.
      if (fill_level_ > num_leds_) {
        CRGB c_tmp = c1_;
        c1_ = c2_;
        c2_ = c_tmp;
        fill_level_ = 0;
        dropping_block_position_ = -block_width_;
        pause_timer_ = kNumPauseFrames;
      } else {
        // Otherwise, update the state for the next frame by moving the dropping block.
        if (dropping_block_position_ + block_width_ == num_leds_ - fill_level_) {
          fill_level_ += block_width_;
          dropping_block_position_ = -block_width_;
        } else {
          dropping_block_position_ += speed_;
          if (dropping_block_position_ + block_width_ > num_leds_ - fill_level_) {
            dropping_block_position_ = num_leds_ - fill_level_ - block_width_;
          }
        }
      }
    }

  private:
    CRGB c1_, c2_;
    int speed_, block_width_;
    int fill_level_, dropping_block_position_;

    int pause_timer_;
};

};

#endif // DROP_H
