#pragma once

#include "esphome/core/gpio.h"
#include "esphome/components/cover/cover.h"


namespace esphome {
namespace ultrasonic_garage {

inline bool compare_floats(float float_a, float float_b, float epsilon = 0.05f) {
    return (fabs(float_a - float_b) < epsilon);
}

class UltrasonicGarageGate : public cover::Cover {
 public:
  void set_activate_pin(GPIOPin *activate_pin) {activate_pin_ = activate_pin; }
  void set_trigger_time(uint32_t trigger_time) {trigger_time_ = trigger_time; }
  void set_operation_timeout(uint32_t operation_timeout) {operation_timeout_ = operation_timeout * 1000; }
  void set_min_position_delta(float min_position_delta) { min_position_delta_ = min_position_delta; }
  float get_setup_priority() const { return setup_priority::DATA; }
  void trigger_gate() { trigger_gate_ = true; }
  bool gate_triggered() { return trigger_gate_; }  
  void dump_config();
  cover::CoverTraits get_traits() override;
  void control(const cover::CoverCall &call) override;
  void update_gate(const uint32_t *time_now);
  void setup_gate();
 protected:
  void handle_gate_trigger_(const uint32_t *time_now);
  cover::CoverOperation handle_gate_direction_();
  void handle_gate_operation_(const uint32_t *time_now);
  GPIOPin *activate_pin_ = nullptr;
  cover::CoverOperation next_direction_;
  cover::CoverOperation operation_direction_;
  bool trigger_gate_ = false;
  float min_position_delta_ = 0.10;
  uint32_t operation_timeout_ = 120000;
  uint32_t trigger_time_ = 400;  
  uint32_t trigger_timer_ = 0;
  uint32_t operation_timer_ = 0;
  bool reverse_required_ = false;
};

} //namespace ultrasonic_garage
} //namespace esphome
