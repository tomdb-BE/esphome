#pragma once

#include "esphome/core/gpio.h"
#include "esphome/components/sensor/sensor.h"

#ifndef ULTRASONIC_US_TO_CM
#define ULTRASONIC_US_TO_CM 57
#endif

namespace esphome {
namespace ultrasonic_garage {

class UltrasonicGarageSonar : public sensor::Sensor, public Component {
 public:
  void set_sonar_type(bool is_car) { is_car_ = is_car; }
  void set_trigger_pin(GPIOPin *trigger_pin) { trigger_pin_ = trigger_pin; }
  void set_echo_pin(InternalGPIOPin *echo_pin) { echo_pin_ = echo_pin; }
  void set_min_distance(uint16_t min_distance) { min_distance_ = min_distance; }
  void set_max_distance(uint16_t max_distance) { max_distance_ = max_distance; }
  void set_min_change(uint16_t min_change) { min_change_ = min_change; }
  void set_max_errors(uint16_t max_errors) { max_errors_ = max_errors; }
  void set_timeout_distance(uint16_t timeout_cm) { timeout_cm_ = timeout_cm; timeout_us_ = timeout_cm * ULTRASONIC_US_TO_CM; }
  void set_sleep_update_interval(uint32_t sleep_update_interval) { sleep_update_interval_ = sleep_update_interval; }
  void set_sleep_timeout(uint32_t sleep_timeout) { sleep_timeout_ = sleep_timeout; }
  void set_pulse_time_us(uint32_t pulse_time_us) { pulse_time_us_ = pulse_time_us; }
  uint16_t get_timeout_cm() { return timeout_cm_; }
  uint16_t get_distance_cm() { return distance_cm_; }
  uint16_t get_previous_distance_cm() { return previous_distance_cm_; }
  bool distance_increasing() { return distance_cm_ && (distance_cm_ > previous_distance_cm_); }
  bool distance_decreasing() { return distance_cm_ && (distance_cm_ < previous_distance_cm_); }
  bool sleeping() {return sleeping_; }
  void enable_sleep() {sleeping_ = true; }
  void disable_sleep() {sleeping_ = false; }
  float get_setup_priority() const { return setup_priority::DATA; }
  void update() { const uint32_t time_now = millis(); update_sensor(&time_now); }
  void update_sensor(const uint32_t *time_now);
  void dump_config() override;
  void send_trigger_pulse();
  void setup_sonar();

 protected:  
  static void IRAM_ATTR interrupt_echo_callback_(uint32_t *interrupt_triggered_var);
  GPIOPin *trigger_pin_;
  InternalGPIOPin *echo_pin_;
  ISRInternalGPIOPin echo_isr_;

  bool is_car_ = false;
  bool sleeping_ = false;

  uint16_t min_distance_ = 0;
  uint16_t max_distance_ = 0;
  uint16_t min_change_ = 0;
  uint16_t max_errors_ = 0;

  uint32_t sleep_update_interval_ = 0;
  uint32_t sleep_timeout_ = 0;
  uint32_t pulse_time_us_ = 0;

  uint16_t timeout_cm_ = 0;
  uint16_t distance_cm_ = 0;
  uint16_t previous_distance_cm_ = 0;
  uint32_t timeout_us_ = 0;
 
  uint32_t pulse_start_us_ = 0;
  uint32_t pulse_end_us_ = 0;
  uint32_t distance_us_ = 0;
  uint32_t previous_distance_us_ = 0;  
};

static UltrasonicGarageSonar* distance_gate_sonar;
static UltrasonicGarageSonar* distance_car_sonar;

} //namespace ultrasonic_garage
} //namespace esphome