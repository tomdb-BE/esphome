#pragma once

#include "esphome/core/gpio.h"
#include "esphome/components/sensor/sensor.h"
#include <esp_timer.h>

#ifndef ULTRASONIC_US_TO_CM
#define ULTRASONIC_US_TO_CM 57
#endif

namespace esphome {
namespace ultrasonic_garage {

class UltrasonicGarageSonar : public sensor::Sensor, public Component {
 public:
  void set_sonar_type(bool is_car) { this->is_car_ = is_car; }
  void set_trigger_pin(GPIOPin *trigger_pin) { this->trigger_pin_ = trigger_pin; }
  void set_echo_pin(InternalGPIOPin *echo_pin) { this->echo_pin_ = echo_pin; }
  void set_min_distance(uint16_t min_distance) { this->min_distance_ = min_distance; }
  void set_max_distance(uint16_t max_distance) { this->max_distance_ = max_distance; }
  void set_min_change(uint16_t min_change) { this->min_change_ = min_change; }
  void set_max_errors(uint16_t max_errors) { this->max_errors_ = max_errors; }
  void set_timeout_distance(uint16_t timeout_cm) { this->timeout_cm_ = timeout_cm; this->pulse_timeout_us_ = timeout_cm * ULTRASONIC_US_TO_CM; this->effect_segment_ = (timeout_cm * 2) / 3; }
  void set_sleep_update_interval(uint32_t sleep_update_interval) { this->sleep_update_interval_ = sleep_update_interval; }
  void set_sleep_timeout(uint32_t sleep_timeout) { this->sleep_timeout_ = sleep_timeout; }
  void set_pulse_time_us(uint32_t pulse_time_us) { this->pulse_time_us_ = pulse_time_us; }
  void set_min_update_interval(uint32_t min_update_interval) { this->min_update_interval_ = min_update_interval; }
  void set_pulse_start_us() { this->pulse_start_us_ = esp_timer_get_time(); }   
  void set_pulse_end_us() { this->pulse_end_us_ = esp_timer_get_time(); }
  void set_sleep_mode(bool sleep_enabled = true) { this->sleep_mode_ = sleep_enabled; }
  void set_sonar_enabled(bool sonar_enabled = true) { this->enabled_ = sonar_enabled; }
  bool read_echo_pin() { return this->echo_pin_->digital_read(); }
  bool get_pulse_started() { return (this->pulse_start_us_ > 0); }
  uint32_t get_distance_cm() { return this->distance_cm_; }
  int32_t get_relative_distance_cm() { return this->relative_distance_cm_; }
  uint32_t get_previous_distance_cm() { return this->previous_distance_cm_; }
  uint32_t get_effect_segment() { return this->effect_segment_; }
  uint32_t get_covered_distance() { return this->max_distance_ - this->min_distance_; }  
  int64_t get_min_update_interval() { return this->min_update_interval_; }
  bool distance_increasing() { return this->distance_cm_ && (this->distance_cm_ > this->previous_distance_cm_); }
  bool distance_decreasing() { return this->distance_cm_ && (this->distance_cm_ < this->previous_distance_cm_); }
  bool is_car() { return this->is_car_; }  
  bool is_sleeping() { return this->sleep_mode_; }
  bool is_enabled() { return this->enabled_; }
  float get_setup_priority() const { return setup_priority::DATA; }
  void update();
  void dump_config() override;
  void setup_sonar();

 protected:
  void send_trigger_pulse_();

  GPIOPin *trigger_pin_;
  InternalGPIOPin *echo_pin_;
  ISRInternalGPIOPin echo_isr_;

  bool is_car_ = false;
  bool sleep_mode_ = true;
  bool enabled_ = false;
  
  uint32_t min_distance_ = 0;
  uint32_t max_distance_ = 200;
  uint32_t min_change_ = 0;
  uint32_t max_errors_ = 0;

  esp_timer_handle_t sleep_timer_;

  int64_t sleep_update_interval_ = 0;
  int64_t sleep_timeout_ = 0;
  int64_t min_update_interval_ = 0;
  
  int64_t pulse_time_us_ = 0;
  int64_t pulse_start_us_ = 0;
  int64_t pulse_end_us_ = 0;
  int64_t pulse_timeout_us_ = 200;
  int64_t pulse_timeout_timer_us_ = 0;

  uint32_t timeout_cm_ = 0;
  uint32_t distance_cm_ = 0;
  uint32_t previous_distance_cm_ = 0;
  int32_t relative_distance_cm_ = -1;

  uint32_t effect_segment_ = 1; 
};

} //namespace ultrasonic_garage
} //namespace esphome