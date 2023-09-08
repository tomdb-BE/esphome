#pragma once

#include "esphome/core/gpio.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/log.h"
#include <esp_timer.h>

namespace esphome {
namespace ultrasonic_garage {

class UltrasonicGarageMotion : public binary_sensor::BinarySensor, public Component {
 public:
  void set_motion_pin(GPIOPin *motion_pin) { motion_pin_ = motion_pin; }
  void set_inverted(bool inverted) { inverted_ = inverted; }
  void set_min_on(uint64_t min_on) { min_on_ = min_on; }
  void set_min_off(uint64_t min_off) { min_off_ = min_off; }  
  void enable_detection() { detection_disabled_ = false; }
  void disable_detection() { detection_disabled_ = true; state = false; esp_timer_stop(motion_sensor_timer_); }
  bool detection_enabled() { return !detection_disabled_; }    
  uint64_t get_on_period() { return (state) ? esp_timer_get_time() - last_period_ : last_period_; }
  uint64_t get_off_period() { return (state) ? last_period_ : esp_timer_get_time() - last_period_; }
  uint64_t get_state_period() { return esp_timer_get_time() - last_period_; }
  bool motion_detected() { return state; }
  float get_setup_priority() const { return setup_priority::DATA; }
  void update() { const int64_t  time_now = esp_timer_get_time(); update_sensor(time_now); }
  static void motion_sensor_timer_callback (void* arg) { UltrasonicGarageMotion *motion_sensor = (UltrasonicGarageMotion *) (arg); motion_sensor->sleeping = false; }
  void update_sensor(const int64_t  time_now);
  void dump_config() override;
  void setup_motion_sensor();
  bool sleeping = true;

 protected:  
  GPIOPin *motion_pin_ = nullptr;
  esp_timer_handle_t motion_sensor_timer_;
  bool inverted_ = false;
  bool detection_disabled_ = false;
  int64_t  last_period_ = esp_timer_get_time();
  uint64_t  min_on_ = 0;
  uint64_t  min_off_ = 0;  
};

} //namespace ultrasonic_garage
} //namespace esphome