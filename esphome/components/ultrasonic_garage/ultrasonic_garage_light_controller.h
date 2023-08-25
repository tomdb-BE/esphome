#pragma once

#include <vector>
#include <list>

#include "esphome/core/component.h"
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/addressable_light_effect.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/light_call.h"
#include "esphome/components/light/automation.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace ultrasonic_garage {

enum UltrasonicGarageActionType {
  OPENING,
  CLOSING,
  DISTANCE_GATE,
  DISTANCE_CAR,
  MOTION,
  IDLE,
  TYPE_COUNT
};

class UltrasonicGarageLightControllerTrigger : public Trigger<> {
 public:
  UltrasonicGarageLightControllerTrigger() {};
};

class UltrasonicGarageLightController {
 public:
  void add_light_action(UltrasonicGarageLightControllerTrigger* trigger, UltrasonicGarageActionType action_type) { light_triggers_[action_type].push_back(trigger); }
  void turn_on() {}
  void setup();
  void activate_triggers(UltrasonicGarageActionType action_type);
 protected:
  std::vector<UltrasonicGarageLightControllerTrigger*> light_triggers_[TYPE_COUNT];
};

class ScanFastLightEffect : public light::AddressableLightEffect, public Component {
 public:
  ScanFastLightEffect(const std::string &name) : AddressableLightEffect(name) {};
  void apply(light::AddressableLight &it, const Color &current_color) override;
 protected:
  uint32_t speed_{10};
  uint16_t width_{50};
};

template<typename... Ts> class StartAction : public Action<Ts...>, public Parented<UltrasonicGarageLightController> {
 public:
  void play(Ts... x) override { this->parent_->turn_on(); }
};

} //namespace ultrasonic_garage
} //namespace esphome