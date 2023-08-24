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
  IDLE
};

class UltrasonicGarageLightControllerTrigger : public Trigger<> {
 public:
  UltrasonicGarageLightControllerTrigger() {}
 protected:
  bool last_on_;
};

struct UltrasonicGarageAction {
  UltrasonicGarageAction(UltrasonicGarageLightControllerTrigger* trigger_in, UltrasonicGarageActionType action_type_in) {
    action_trigger = trigger_in;
    action_type = action_type_in;
  };
  UltrasonicGarageLightControllerTrigger* action_trigger;
  UltrasonicGarageActionType action_type;
};

class UltrasonicGarageLightController {
 public:
  void add_light_action(UltrasonicGarageAction light_action) { light_actions_.push_back(light_action); }
  void setup();
  void turn_on() { 
    for (auto light_action : light_actions_) {
       light_action.action_trigger->trigger();
    }
  }
 protected:
  std::list<UltrasonicGarageAction> light_actions_;  
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