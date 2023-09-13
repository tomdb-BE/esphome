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
  void set_sleeping(bool sleep_enabled) { this->sleeping_ = sleep_enabled; }
  void enable_detection() { this->detection_disabled_ = false; }
  void disable_detection() { this->detection_disabled_ = true; this->state = false; esp_timer_stop(this->motion_sensor_timer_); }
  bool detection_enabled() { return !this->detection_disabled_; }    
  uint64_t get_on_period() { return (this->state) ? esp_timer_get_time() - this->last_period_ : this->last_period_; }
  uint64_t get_off_period() { return (this->state) ? this->last_period_ : esp_timer_get_time() - this->last_period_; }
  uint64_t get_state_period() { return esp_timer_get_time() - this->last_period_; }
  bool motion_detected() { return this->state; }
  bool is_sleeping() { return this->sleeping_; }
  float get_setup_priority() const { return setup_priority::DATA; }
  void update() { this->update_sensor(); }
  void update_sensor();
  void dump_config() override;
  void setup_motion_sensor();
  

 protected:  
  GPIOPin *motion_pin_ = nullptr;
  esp_timer_handle_t motion_sensor_timer_;
  bool inverted_ = false;
  bool detection_disabled_ = false;
  bool sleeping_ = false;
  int64_t  last_period_ = esp_timer_get_time();
  uint64_t  min_on_ = 0;
  uint64_t  min_off_ = 0;  
};

} //namespace ultrasonic_garage
} //namespace esphome