#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/components/sensor/sensor.h"

#ifndef ULTRASONIC_US_TO_CM
#define ULTRASONIC_US_TO_CM 57
#endif

namespace esphome {
namespace ultrasonic_interrupt {

class UltrasonicInterruptSensorComponent : public sensor::Sensor, public PollingComponent {
 public:  
  void set_trigger_pin(GPIOPin *trigger_pin) { this->trigger_pin_ = trigger_pin; }
  void set_echo_pin(InternalGPIOPin *echo_pin) { this->echo_pin_ = echo_pin; }
  void set_timeout(uint32_t timeout_m) { this->timeout_cm_ = timeout_m * 100; this->timeout_us_ = this->timeout_cm_ * ULTRASONIC_US_TO_CM; }
  void set_pulse_time_us(uint32_t pulse_time_us) { this->pulse_time_us_ = pulse_time_us; }
  uint16_t get_timeout_cm() { return this->timeout_cm_;}
  uint16_t get_distance_cm() { return this->distance_cm_;}
  uint16_t get_previous_distance_cm() { return this->previous_distance_cm_;}
  bool distance_increasing() { return this->distance_cm_ && (this->distance_cm_ > this->previous_distance_cm_); }
  bool distance_decreasing() { return this->distance_cm_ && (this->distance_cm_ < this->previous_distance_cm_); }
  bool sleeping() {return this->sleeping_; }
  void enable_sleep() {this->sleeping_ = true; }
  void disable_sleep() {this->sleeping_ = false; }
  float get_setup_priority() const { return setup_priority::DATA; }
  void setup() override;
  void update() override;
  void dump_config() override;
  void send_trigger_pulse();

 protected:  
  static void IRAM_ATTR interrupt_echo_callback_(UltrasonicInterruptSensorComponent *sonar);
  GPIOPin *trigger_pin_;
  InternalGPIOPin *echo_pin_;
  ISRInternalGPIOPin echo_isr_;
  uint16_t timeout_cm_;
  uint16_t distance_cm_ = 0;
  uint16_t previous_distance_cm_ = 0;
  uint32_t timeout_us_;
  uint32_t pulse_time_us_;
  uint32_t pulse_start_us_ = 0;
  uint32_t distance_us_ = 0;
  uint32_t previous_distance_us_ = 0;
  bool sleeping_ = false;  
};

} //namespace ultrasonic_interrupt
} //namespace esphome
