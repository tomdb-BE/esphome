#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/components/cover/cover.h"
#include "sonar.h"

#ifndef ULTRASONIC_US_TO_CM
#define ULTRASONIC_US_TO_CM 57
#endif

namespace esphome {
namespace ultrasonic_cover {

class UltrasonicCover : public cover::Cover, public Component {
 public:
  void set_activate_pin(GPIOPin *activate_pin) {activate_pin = activate_pin_; }
  void set_active_pin(InternalGPIOPin *active_pin) {active_pin = active_pin_; }
  void set_sonar_door(Sonar sonar_door) {sonar_door_ = sonar_door; }
  void set_sonar_car(Sonar sonar_car) {sonar_car_ = sonar_car; }
  void setup() override;
  void dump_config() override;
  cover::CoverTraits get_traits() override;
  void control(const cover::CoverCall &call) override;
 protected:
  GPIOPin *activate_pin_;
  InternalGPIOPin *active_pin_ = nullptr;
  Sonar sonar_door_;
  Sonar sonar_car_;
};


} //namespace ultrasonic_cover
} //namespace esphome
