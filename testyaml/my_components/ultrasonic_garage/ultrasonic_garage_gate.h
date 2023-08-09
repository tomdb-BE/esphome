#pragma once

#include "esphome/core/gpio.h"
#include "esphome/components/cover/cover.h"


namespace esphome {
namespace cover {
namespace ultrasonic_garage {

inline bool compare_floats(float float_a, float float_b, float epsilon = 0.05f) {
    return (fabs(float_a - float_b) < epsilon);
}

class UltrasonicGarageGate : public Cover, public Component {
 public:
  void set_activate_pin(GPIOPin *activate_pin) {activate_pin_ = activate_pin; }
  void set_active_pin(GPIOPin *active_pin) {active_pin_ = active_pin; active_pin_set_ = true;}
  void set_trigger_time(uint32_t trigger_time) {trigger_time_ = trigger_time; }
  void set_operation_timeout(uint32_t operation_timeout) {operation_timeout_ = operation_timeout * 1000; }
  void set_min_position_delta(float min_position_delta) { min_position_delta_ = min_position_delta; }
  void dump_config() override;
  CoverTraits get_traits() override;
  void control(const CoverCall &call) override;
  void update_gate();
  void setup_gate();
  void trigger_gate();
 protected:
  void handle_gate_trigger_();
  void handle_gate_operation_();
  GPIOPin *activate_pin_;
  GPIOPin *active_pin_;
  CoverOperation next_direction_;
  CoverOperation operation_direction_;
  bool active_pin_set_ = false;
  float min_position_delta_ = 0.10;
  uint32_t operation_timeout_ = 120000;
  uint32_t trigger_time_ = 400;
  uint32_t reference_timer_ = 0;
  uint32_t trigger_timer_ = 0;
  uint32_t operation_timer_ = 0;
  bool reverse_required_ = false;
};

} //namespace ultrasonic_garage
} //namespace cover
} //namespace esphome
