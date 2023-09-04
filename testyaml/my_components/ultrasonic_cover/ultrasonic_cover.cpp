#include "ultrasonic_cover.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_cover {

static const char *const TAG = "ultrasonic.cover";

void UltrasonicCover::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Ultrasonic Cover...");
  activate_pin_->setup();
  active_pin_->setup();  
}

void UltrasonicCover::dump_config() {
  LOG_COVER("", "Ultrasonic Cover", this);
  LOG_PIN("  Activate Pin: ", activate_pin_);
  LOG_PIN("  Active Pin: ", active_pin_);
  ESP_LOGCONFIG(TAG, "Config done");
}

cover::CoverTraits UltrasonicCover::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(false);
  traits.set_supports_position(true);
  traits.set_supports_tilt(false);
  traits.set_supports_stop(true);
  return traits;
}

void UltrasonicCover::control(const cover::CoverCall &call) {
  // This will be called every time the user requests a state change.
  if (call.get_position().has_value()) {
    float pos = *call.get_position();
    // Write pos (range 0-1) to cover
    // ...

    // Publish new state
    position = pos;
    publish_state();
  }
  if (call.get_stop()) {
    // User requested cover stop
  }
}

} //namespace ultrasonic_cover
} //namespace esphome