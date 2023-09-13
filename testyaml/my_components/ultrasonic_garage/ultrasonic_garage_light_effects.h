#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/addressable_light_effect.h"
#include "ultrasonic_garage_gate.h"
#include "ultrasonic_garage_sonar.h"

namespace esphome {
namespace ultrasonic_garage {

class UltrasonicGarageLightEffect : public light::AddressableLightEffect {
 public:
  UltrasonicGarageLightEffect(const std::string &name) : AddressableLightEffect(name) {};
  void set_mirrored(bool mirrored) { this->mirrored_ = mirrored; }
  void set_reversed(bool reversed) { this->reversed_ = reversed; }
  virtual void start_extra() {}
  virtual void start();  
 protected:
  bool mirrored_ = false;
  bool reversed_ = false;
  uint16_t progress_ = 0;
  uint16_t led_count_ = 0;  
};

class ScanFastLightEffect : public UltrasonicGarageLightEffect {
 public:
  ScanFastLightEffect(const std::string &name) : UltrasonicGarageLightEffect(name) {};
  void start_extra() override;
  void apply(light::AddressableLight &it, const Color &current_color) override;  
};

class FillFastLightEffect : public UltrasonicGarageLightEffect {
 public:
  FillFastLightEffect(const std::string &name) : UltrasonicGarageLightEffect(name) {};  
  void apply(light::AddressableLight &it, const Color &current_color) override;
};

class DistanceGateLightEffect : public UltrasonicGarageLightEffect {
 public:
  DistanceGateLightEffect(const std::string &name) : UltrasonicGarageLightEffect(name) {};
  void set_gate(UltrasonicGarageGate* gate) { this->gate_ = gate; }
  void start_extra() override;
  void apply(light::AddressableLight &it, const Color &current_color) override;
 protected:
  UltrasonicGarageGate* gate_{nullptr};
  float previous_position_ = 0.00f;
};

class DistanceCarLightEffect : public UltrasonicGarageLightEffect {
 public:
  DistanceCarLightEffect(const std::string &name) : UltrasonicGarageLightEffect(name) {};  
  void set_sonar_sensor(UltrasonicGarageSonar* sonar_sensor) { this->sonar_sensor_ = sonar_sensor; }
  void start_extra() override;
  void apply(light::AddressableLight &it, const Color &current_color) override;
 protected:
  UltrasonicGarageSonar* sonar_sensor_{nullptr};
  uint32_t previous_distance_ = 0;
  uint32_t segment_ = 1;
};

} //namespace ultrasonic_garage
} //namespace esphome