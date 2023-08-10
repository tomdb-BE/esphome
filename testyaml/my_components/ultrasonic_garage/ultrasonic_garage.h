#pragma once

#include "esphome/core/component.h"
#include "ultrasonic_garage_gate.h"
#include "ultrasonic_garage_sonar.h"
#include "ultrasonic_garage_motion.h"

namespace esphome {
namespace ultrasonic_garage {

class UltrasonicGarage : public PollingComponent {
 public:
  void set_gate(UltrasonicGarageGate *gate) { gate_ = gate; }
  void set_sonar_gate(UltrasonicGarageSonar *sonar_gate) { sonar_gate_ = sonar_gate; }
  void set_sonar_car(UltrasonicGarageSonar *sonar_car) { sonar_car_ = sonar_car; }
  void set_motion_sensor(UltrasonicGarageMotion *motion_sensor) { motion_sensor_ = motion_sensor; }  
  void dump_config() override;  
  void setup();
  void update();
 protected:
  UltrasonicGarageGate *gate_;
  UltrasonicGarageSonar *sonar_gate_;
  UltrasonicGarageSonar *sonar_car_;
  UltrasonicGarageMotion *motion_sensor_;
};

} //namespace ultrasonic_garage
} //namespace esphome