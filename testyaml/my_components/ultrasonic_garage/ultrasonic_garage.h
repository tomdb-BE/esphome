#pragma once

#include "esphome/core/component.h"
#include "ultrasonic_garage_gate.h"
#include "ultrasonic_garage_sonar.h"
#include "ultrasonic_garage_motion.h"
#include "ultrasonic_garage_light_controller.h"

namespace esphome {
namespace ultrasonic_garage {

class UltrasonicGarage : public PollingComponent {
 public:
  UltrasonicGarage() : PollingComponent() {}
  ~UltrasonicGarage() {
    delete light_controller_;
  }
  void set_gate(UltrasonicGarageGate *gate) { gate_ = gate; }
  void set_sonar_gate(UltrasonicGarageSonar *sonar_gate) { sonar_gate_ = sonar_gate; }
  void set_sonar_car(UltrasonicGarageSonar *sonar_car) { sonar_car_ = sonar_car; }
  void set_motion_sensor(UltrasonicGarageMotion *motion_sensor) { motion_sensor_ = motion_sensor; }
  void set_gate_sensor(UltrasonicGarageMotion *gate_sensor) { gate_sensor_ = gate_sensor; }
  void set_light_controller(UltrasonicGarageLightController *light_controller) { light_controller_ = light_controller; }
  float get_setup_priority() const override { return esphome::setup_priority::DATA; }
  void dump_config() override;  
  void setup();
  void update();
 protected:
  UltrasonicGarageGate *gate_;
  UltrasonicGarageSonar *sonar_gate_;
  UltrasonicGarageSonar *sonar_car_;
  UltrasonicGarageMotion *gate_sensor_;
  UltrasonicGarageMotion *motion_sensor_;
  UltrasonicGarageLightController *light_controller_;
};

} //namespace ultrasonic_garage
} //namespace esphome