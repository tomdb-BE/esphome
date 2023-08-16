#pragma once

#include "esphome/core/component.h"
#include "ultrasonic_garage_gate.h"
#include "ultrasonic_garage_sonar.h"
#include "ultrasonic_garage_motion.h"
#include "ultrasonic_garage_lights.h"

namespace esphome {
namespace ultrasonic_garage {

class UltrasonicGarage : public PollingComponent {
 public:
  void set_gate(UltrasonicGarageGate *gate) { gate_ = gate; }
  void set_sonar_gate(UltrasonicGarageSonar *sonar_gate) { sonar_gate_ = sonar_gate; }
  void set_sonar_car(UltrasonicGarageSonar *sonar_car) { sonar_car_ = sonar_car; }
  void set_motion_sensor(UltrasonicGarageMotion *motion_sensor) { motion_sensor_ = motion_sensor; }
  void set_gate_sensor(UltrasonicGarageMotion *gate_sensor) { gate_sensor_ = gate_sensor; }
  void set_lights(UltrasonicGarageLights *lights) { lights_ = lights; }
  void dump_config() override;  
  void setup();
  void update();
 protected:
  UltrasonicGarageGate *gate_;
  UltrasonicGarageSonar *sonar_gate_;
  UltrasonicGarageSonar *sonar_car_;
  UltrasonicGarageMotion *gate_sensor_;
  UltrasonicGarageMotion *motion_sensor_;
  UltrasonicGarageLights *lights_;
};

} //namespace ultrasonic_garage
} //namespace esphome