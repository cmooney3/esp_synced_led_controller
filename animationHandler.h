// All the various animations themselves
#include "animations/scan.h"

// Here we define the list of animations that the controller can play
// by building up an enum full of their names and a generator function
// that returns a generic Animation* give the type of animation.
// When adding a new animation, this is where you do the book-keeping.
enum AnimationType {ANIM_SCAN, NUM_ANIMATION_TYPES} typedef AnimationType;
Animation* buildNewAnimation(AnimationType type) {
  switch (type) {
    case AnimationType::ANIM_SCAN:
      return new Scan::ScanAnimation(leds, kNumLEDs);
    default:
      return NULL;
  }
}
Animation* current_animation;


void SwitchToNextAnimation() {
  static uint8_t next_animation_type = 0;
  // First, avoid memory leaks by deleting the animation that's ending.
  if (current_animation) {
    delete current_animation;
  }

  // Create a new animation object of the next type.
  current_animation = buildNewAnimation(
                          static_cast<AnimationType>(next_animation_type));
  Serial.printf("New animation Started (type: %d)\n\r", next_animation_type);

  // Advance to the next animation.
  next_animation_type = (next_animation_type + 1) % NUM_ANIMATION_TYPES;
}

extern painlessMesh mesh;
void renderNextFrame() {
  // Render the next frame
  current_animation->generateFrame(mesh.getNodeTime());
  FastLED.show();

  // If that was the last frame of the animation, queue up the next one
  //if (!has_more_frames) {
  //  SwitchToNextAnimation();
  //}
}

