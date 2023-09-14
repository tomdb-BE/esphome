#pragma once

#include <vector>

#include "esphome/core/automation.h"
#include "esphome/components/light/automation.h"
#include "esphome/components/light/light_state.h"
#include "ultrasonic_garage_light_effects.h"

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
  UltrasonicGarageLightController () {}
  ~UltrasonicGarageLightController () {}  
  void add_light_states(std::vector<light::LightState*> light_states) { this->light_states_ = light_states; }
  void turn_on_all() { for (auto light_state : this->light_states_) light_state->turn_on(); }
  void turn_off_all() { for (auto light_state : this->light_states_) light_state->turn_off(); }
  void setup();  
  void add_light_trigger(UltrasonicGarageLightControllerTrigger* light_trigger, UltrasonicGarageActionType action_type);
  void activate_triggers(UltrasonicGarageActionType action_type);  
 protected:
  std::vector<light::LightState*> light_states_;
  std::vector<UltrasonicGarageLightControllerTrigger*> light_triggers_[TYPE_COUNT];
};

} //namespace ultrasonic_garage
} //namespace esphome