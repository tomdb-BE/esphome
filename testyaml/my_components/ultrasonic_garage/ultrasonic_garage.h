#pragma once

#include "esphome/core/component.h"
#include "ultrasonic_garage_gate.h"
#include "ultrasonic_garage_sonar.h"

namespace esphome {
namespace cover {
namespace ultrasonic_garage {

class UltrasonicGarage : public PollingComponent {
 public:
  void set_gate(UltrasonicGarageGate *gate) { gate_ = gate; }
  void set_sonar_gate(UltrasonicGarageSonar *sonar_gate) { sonar_gate_ = sonar_gate; }
  void set_sonar_car(UltrasonicGarageSonar *sonar_car) { sonar_car_ = sonar_car; }
  void dump_config() override;
  void setup() override;
  void update();
 protected:
  UltrasonicGarageGate *gate_;
  UltrasonicGarageSonar *sonar_gate_;
  UltrasonicGarageSonar *sonar_car_;
  bool is_ready_ = false;
};

} //namespace ultrasonic_garage
} //namespace cover
} //namespace esphome