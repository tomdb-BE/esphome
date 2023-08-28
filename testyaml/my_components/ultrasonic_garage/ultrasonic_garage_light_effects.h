#pragma once

#include "esphome/core/component.h"
#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/addressable_light_effect.h"
#include "ultrasonic_garage_sonar.h"

namespace esphome {
namespace ultrasonic_garage {

class UltrasonicGarageLightEffect : public light::AddressableLightEffect {
 public:
  UltrasonicGarageLightEffect(const std::string &name) : AddressableLightEffect(name) {};
  void set_mirrored(bool mirrored) { mirrored_ = mirrored; }
  void set_reversed(bool reversed) { reversed_ = reversed; }
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
  void set_sonar_sensor(UltrasonicGarageSonar* sonar_sensor) { sonar_sensor_ = sonar_sensor; }  
  void apply(light::AddressableLight &it, const Color &current_color) override;
 protected:
  UltrasonicGarageSonar* sonar_sensor_ = nullptr;
};

class DistanceCarLightEffect : public UltrasonicGarageLightEffect {
 public:
  DistanceCarLightEffect(const std::string &name) : UltrasonicGarageLightEffect(name) {};
  void set_sonar_sensor(UltrasonicGarageSonar* sonar_sensor) { sonar_sensor_ = sonar_sensor; }   
  void apply(light::AddressableLight &it, const Color &current_color) override;
 protected:
  UltrasonicGarageSonar* sonar_sensor_ = nullptr;
};

} //namespace ultrasonic_garage
} //namespace esphome