#include "ultrasonic_garage_light_controller.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.light_controller";

void UltrasonicGarageLightController::setup() {
  return;
}

void UltrasonicGarageLightController::activate_triggers(UltrasonicGarageActionType action_type) {
  if (light_triggers_[action_type].empty())
    return;
  for (UltrasonicGarageLightControllerTrigger* light_trigger : light_triggers_[action_type]) {
    light_trigger->trigger();
  }
}

void ScanFastLightEffect::apply(light::AddressableLight &it, const Color &current_color) {
  light::ESPHSVColor hsv;
  hsv.value = 255;
  hsv.saturation = 240;
  uint16_t hue = (millis() * this->speed_) % 0xFFFF;
  const uint16_t add = 0xFFFF / this->width_;
  for (auto var : it) {
    hsv.hue = hue >> 8;
    var = hsv;
    hue += add;
  }
  it.schedule_show();
}

} //namespace ultrasonic_garage
} //namespace esphome