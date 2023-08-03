#pragma once
#include "esphome/components/ultrasonic/ultrasonic_sensor.h"
#include "NewPing.h"

namespace esphome {
namespace ultrasonic_newping {

class UltrasonicNewpingSensorComponent : public ultrasonic::UltrasonicSensorComponent {
 public:
  void setup() override;
  void update() override;

 protected:
  NewPing *sonar;
  //NewPing newping;
};

} //namespace ultrasonic_newping
} //namespace esphome
