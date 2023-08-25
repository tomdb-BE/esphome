#pragma once

#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/addressable_light_effect.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/light/light_call.h"
#include "esphome/components/light/automation.h"
#include "esphome/core/automation.h"
#include "ultrasonic_garage_sonar.h"

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
  ~UltrasonicGarageLightController () {
    for (uint8_t i = 0; i < TYPE_COUNT; i++) {
      for (UltrasonicGarageLightControllerTrigger* light_trigger : light_triggers_[i]) {
        delete light_trigger;
      }
    }
  }
  void add_light_action(UltrasonicGarageLightControllerTrigger* trigger, UltrasonicGarageActionType action_type) { light_triggers_[action_type].push_back(trigger); }
  void activate_triggers(UltrasonicGarageActionType action_type);
 protected:
  std::vector<UltrasonicGarageLightControllerTrigger*> light_triggers_[TYPE_COUNT];
};

class ScanFastLightEffect : public light::AddressableLightEffect, public Component {
 public:
  ScanFastLightEffect(const std::string &name) : AddressableLightEffect(name) {};
  void set_mirrored(bool mirrored) { mirrored_ = mirrored; }
  void set_reversed(bool reversed) { reversed_ = reversed; }
  void start() override;
  void apply(light::AddressableLight &it, const Color &current_color) override;
 protected:
  bool mirrored_ = false;
  bool reversed_ = false;
  uint16_t progress_ = 0;
  uint16_t led_count_ = 0;
};

class FillFastLightEffect : public light::AddressableLightEffect, public Component {
 public:
  FillFastLightEffect(const std::string &name) : AddressableLightEffect(name) {};
  void set_mirrored(bool mirrored) { mirrored_ = mirrored; }
  void set_reversed(bool reversed) { reversed_ = reversed; }
  void start() override;
  void apply(light::AddressableLight &it, const Color &current_color) override;
 protected:
  bool mirrored_ = false;
  bool reversed_ = false;
  uint16_t progress_ = 0;
  uint16_t led_count_ = 0;
};

class GateDistanceLightEffect : public light::AddressableLightEffect, public Component {
 public:
  GateDistanceLightEffect(const std::string &name) : AddressableLightEffect(name) {};
  void set_sonar_sensor(UltrasonicGarageSonar* sonar_sensor) { sonar_sensor_ = sonar_sensor; }
  void set_mirrored(bool mirrored) { mirrored_ = mirrored; } 
  void start() override;
  void apply(light::AddressableLight &it, const Color &current_color) override;
 protected:
  UltrasonicGarageSonar* sonar_sensor_ = nullptr;
  bool mirrored_ = false;    
  uint16_t led_count_ = 0;
};

class CarDistanceLightEffect : public light::AddressableLightEffect, public Component {
 public:
  CarDistanceLightEffect(const std::string &name) : AddressableLightEffect(name) {};
  void set_sonar_sensor(UltrasonicGarageSonar* sonar_sensor) { sonar_sensor_ = sonar_sensor; }  
  void set_mirrored(bool mirrored) { mirrored_ = mirrored; }  
  void start() override;
  void apply(light::AddressableLight &it, const Color &current_color) override;
 protected:
  UltrasonicGarageSonar* sonar_sensor_ = nullptr;
  bool mirrored_ = false;    
  uint16_t led_count_ = 0;
};

} //namespace ultrasonic_garage
} //namespace esphome