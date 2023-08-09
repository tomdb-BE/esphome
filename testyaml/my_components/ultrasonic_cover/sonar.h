#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

#ifndef ULTRASONIC_US_TO_CM
#define ULTRASONIC_US_TO_CM 57
#endif

namespace esphome {
namespace ultrasonic_cover {

class Sonar : public sensor::Sensor, public PollingComponent {
 public:  
  void set_trigger_pin(GPIOPin *trigger_pin) { trigger_pin_ = trigger_pin; }
  void set_echo_pin(InternalGPIOPin *echo_pin) { echo_pin_ = echo_pin; }
  void set_min_distance(uint16 min_distance) { min_distance_ = min_distance; }
  void set_max_distance(uint16 max_distance) { max_distance_ = max_distance; }
  void set_min_change(uint16 min_change) { min_change_ = min_change; }
  void set_errors_ignored(uint16 errors_ignored) { errors_ignored_ = errors_ignored; }
  void set_timeout_distance(uint16 timeout_m) { timeout_cm_ = timeout_m * 100; timeout_us_ = timeout_cm_ * ULTRASONIC_US_TO_CM; }
  void set_sleep_update_interval(uint32_t sleep_update_interval) { sleep_update_interval_ = sleep_update_interval; }
  void set_sleep_timeout(uint32_t sleep_timeout) { sleep_timeout_ = sleep_timeout; }
  void set_pulse_time_us(uint32_t pulse_time_us) { pulse_time_us_ = pulse_time_us; }
  void set_timeout(uint32_t timeout_m) { timeout_cm_ = timeout_m * 100; timeout_us_ = timeout_cm_ * ULTRASONIC_US_TO_CM; }
  
  uint16_t get_timeout_cm() { return timeout_cm_; }
  uint16_t get_distance_cm() { return distance_cm_; }
  uint16_t get_previous_distance_cm() { return previous_distance_cm_; }
  bool distance_increasing() { return distance_cm_ && (distance_cm_ > previous_distance_cm_); }
  bool distance_decreasing() { return distance_cm_ && (distance_cm_ < previous_distance_cm_); }
  bool sleeping() {return sleeping_; }
  void enable_sleep() {sleeping_ = true; }
  void disable_sleep() {sleeping_ = false; }
  float get_setup_priority() const { return setup_priority::DATA; }
  void setup() override;
  void update() override;
  void dump_config() override;
  void send_trigger_pulse();
  ISRInternalGPIOPin echo_isr_;

 protected:  
  static void IRAM_ATTR interrupt_echo_callback_(Sonar *sonar);
  GPIOPin *trigger_pin_;
  InternalGPIOPin *echo_pin_;

  uint16_t min_distance_;
  uint16_t max_distance_;
  uint16_t min_change_;
  uint16_t errors_ignored_;

  uint32_t sleep_update_interval_;
  uint32_t sleep_timeout_;
  uint32_t pulse_time_us_;

  uint16_t timeout_cm_;
  uint16_t distance_cm_ = 0;
  uint16_t previous_distance_cm_ = 0;
  uint32_t timeout_us_;
 
  uint32_t pulse_start_us_ = 0;
  uint32_t distance_us_ = 0;
  uint32_t previous_distance_us_ = 0;
  bool sleeping_ = false;  
};

} //namespace sonar_sensor
} //namespace esphome
