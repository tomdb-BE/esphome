#include "ultrasonic_garage_lights.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.lights";

void UltrasonicGarageLights::setup() {
  return;
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