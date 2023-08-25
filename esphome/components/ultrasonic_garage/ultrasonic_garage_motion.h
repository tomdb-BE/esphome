#pragma once

#include "esphome/core/gpio.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace ultrasonic_garage {

class UltrasonicGarageMotion : public binary_sensor::BinarySensor, public Component {
 public:
  void set_motion_pin(GPIOPin *motion_pin) { motion_pin_ = motion_pin; }
  void set_inverted(bool inverted) { inverted_ = inverted; }
  void set_min_on(uint32_t min_on) { min_on_ = min_on; }
  void set_min_off(uint32_t min_off) { min_off_ = min_off; }  
  void enable_detection() { detection_enabled_ = true; }
  void disable_detection() { detection_enabled_ = false; motion_detected_ = false; timeout_timer_ = 0; }
  bool detection_enabled() { return detection_enabled_; }    
  uint32_t get_on_period() { return (motion_detected_) ? millis() - last_period_ : last_period_; }
  uint32_t get_off_period() { return (motion_detected_) ? last_period_ : millis() - last_period_; }
  uint32_t get_state_period() { return millis() - last_period_; }
  bool motion_detected() { return motion_detected_; }
  float get_setup_priority() const { return setup_priority::DATA; }
  void update() { const uint32_t time_now = millis(); update_sensor(&time_now); }
  void update_sensor(const uint32_t *time_now);
  void dump_config() override;
  void setup_motion_sensor();

 protected:  
  GPIOPin *motion_pin_ = nullptr;
  bool inverted_ = false; 
  bool motion_detected_ = false;
  bool detection_enabled_ = true; 
  uint32_t min_on_ = 0;
  uint32_t min_off_ = 0;
  uint32_t timeout_timer_ = 0;
  uint32_t last_period_ = millis();
};

} //namespace ultrasonic_garage
} //namespace esphome