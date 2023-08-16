#pragma once

#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/light/addressable_light_effect.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/light_call.h"

namespace esphome {
namespace ultrasonic_garage {

class UltrasonicGarageLights : public Component {
 public:
  void set_light(light::AddressableLightState *light) { light_ = light; }
  void setup() override;  
  void turn_on() { light_->make_call().set_state(true).perform(); }
  void turn_off() { light_->make_call().set_state(false).perform(); }
  void toggle() { light_->make_call().set_state(!light_->remote_values.is_on()).perform(); }
  void set_effect(const std::string effect) { light_->make_call().set_effect(effect).perform(); }
 protected:
  light::AddressableLightState *light_;

};

class ScanFastLightEffect : public light::AddressableLightEffect, public Component {
 public:
  ScanFastLightEffect(const std::string &name) : AddressableLightEffect(name) {};
  void apply(light::AddressableLight &it, const Color &current_color) override;
 protected:
  uint32_t speed_{10};
  uint16_t width_{50};
};

} //namespace ultrasonic_garage
} //namespace esphome