#pragma once

#include "esphome/core/gpio.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include <esp_timer.h>

namespace esphome {
namespace ultrasonic_garage {

class UltrasonicGarageMotion : public binary_sensor::BinarySensor, public Component {
 public:
  void set_motion_pin(GPIOPin *motion_pin) { this->motion_pin_ = motion_pin; }
  void set_inverted(bool inverted) { this->inverted_ = inverted; }
  void set_min_on(uint64_t min_on) { this->min_on_ = min_on; }
  void set_min_off(uint64_t min_off) { this->min_off_ = min_off; }  
  void set_detection(bool detection_enabled = true) { this->enabled_ = detection_enabled; }  
  bool detection_enabled() { return this->enabled_; }    
  uint64_t get_on_period() { return (this->state) ? esp_timer_get_time() - this->last_period_ : this->last_period_; }
  uint64_t get_off_period() { return (this->state) ? this->last_period_ : esp_timer_get_time() - this->last_period_; }
  uint64_t get_state_period() { return esp_timer_get_time() - this->last_period_; }
  bool motion_detected() { return this->state; }
  bool is_enabled() { return this->enabled_; }
  float get_setup_priority() const { return setup_priority::DATA; }
  void setup() override;
  bool update();
  void dump_config() override;
  
 protected:  
  GPIOPin *motion_pin_ = nullptr;  
  bool enabled_ = false;
  bool inverted_ = false;
  bool previous_measurement_ = false;
  int64_t new_state_timer_ = 0;
  int64_t last_period_ = esp_timer_get_time();
  int64_t min_on_ = 0;
  int64_t min_off_ = 0;    
};

} //namespace ultrasonic_garage
} //namespace esphome