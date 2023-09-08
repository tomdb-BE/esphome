#include "ultrasonic_garage_gate.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.gate";

void UltrasonicGarageGate::setup_gate() {
  activate_pin_->setup();
}

void UltrasonicGarageGate::dump_config() {
  const char *const TAG = "ultrasonicgarage";
  LOG_COVER("  ", "Gate", this);
  LOG_PIN("    Activate Pin: ", activate_pin_);
  ESP_LOGCONFIG(TAG, "    Min. position delta: %d%%", (int) (min_position_delta_ * 100));
  ESP_LOGCONFIG(TAG, "    Trigger time: %dms", (int) trigger_time_);
  ESP_LOGCONFIG(TAG, "    Operation timeout: %ds", (int) (operation_timeout_ / 1000));
}

void UltrasonicGarageGate::update_gate(const uint32_t time_now) {
  //ESP_LOGD(TAG, "Time NOW = %d", *time_now);
  if (trigger_gate_)
    handle_gate_trigger_(time_now);
  if (current_operation != cover::COVER_OPERATION_CLOSING)
    handle_gate_operation_(time_now);
}

cover::CoverTraits UltrasonicGarageGate::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(false);
  traits.set_supports_position(true);
  traits.set_supports_tilt(false);
  traits.set_supports_stop(true);
  return traits;
}

void UltrasonicGarageGate::control(const cover::CoverCall &call) {
  // This will be called every time the user requests a state change.  
  if (call.get_position().has_value()) {
    float pos = *call.get_position();
    ESP_LOGD(TAG, "Cover activated - Position: %f", pos);
    // If position change smaller than minimum percentage, do nothing.
    if (fabs(pos - position) < min_position_delta_)
      return;
    if (pos > position) {
      current_operation = cover::COVER_OPERATION_CLOSING;
    }
    else {
      current_operation = cover::COVER_OPERATION_OPENING;
    }
    operation_direction_ = current_operation;
    trigger_gate_ = true;
    // Publish new state    
    position_delta = fabs(position - pos);
    position = pos;    
    publish_state();
  }
  if (call.get_stop()) {
    operation_direction_ = cover::COVER_OPERATION_IDLE;
    trigger_gate_ = true;
    // User requested cover stop
  }
}

void UltrasonicGarageGate::handle_gate_trigger_(const uint32_t time_now) {
  //ESP_LOGD(TAG, "Time NOW FUNC = %d", *time_now);
  if (!trigger_timer_) {    
    activate_pin_->digital_write(true);
    trigger_timer_ = time_now + trigger_time_;
    ESP_LOGD(TAG, "Time NOW = %d", (int) time_now);
    ESP_LOGD(TAG, "Time NOW TRIGGER = %d", (int) trigger_time_);    
    ESP_LOGD(TAG, "Gate switch ON: timeout = %d", (int) trigger_timer_);    
  }
  else if (time_now > trigger_timer_) {  
    trigger_gate_ = false;   
    trigger_timer_ = 0;
    activate_pin_->digital_write(false);    
    ESP_LOGD(TAG, "Gate switch OFF");    
    next_direction_ = handle_gate_direction_();    
  }  
}

cover::CoverOperation UltrasonicGarageGate::handle_gate_direction_() {
    if (operation_direction_ == cover::COVER_OPERATION_IDLE)
      return (next_direction_ == cover::COVER_OPERATION_CLOSING) ? cover::COVER_OPERATION_OPENING : cover::COVER_OPERATION_CLOSING;
    if (reverse_required_) {
      ESP_LOGD(TAG, "Triggering the gate once more in the right direction after reversing");
      reverse_required_ = false; 
      trigger_gate_ = true;
      return operation_direction_;      
    }
    if (operation_direction_ != next_direction_) {
      ESP_LOGD(TAG, "Stopping gate to reverse direction");
      reverse_required_ = true;
      trigger_gate_ = true;
      return next_direction_;
    }
    else {      
      ESP_LOGD(TAG, "Gate action completed, gate trigger OFF");
      reverse_required_ = false; 
      return (operation_direction_ == cover::COVER_OPERATION_CLOSING) ? cover::COVER_OPERATION_OPENING : cover::COVER_OPERATION_CLOSING;
    }
}

void UltrasonicGarageGate::handle_gate_operation_(const uint32_t time_now) {
  if (!operation_timer_) {
    operation_timer_ = time_now + operation_timeout_;
    ESP_LOGD(TAG, "Gate operation timer started");
  }  
  else if (time_now > operation_timer_) {
    operation_timer_ = 0;
    ESP_LOGD(TAG, "Gate operation timeout");
  }  
}


} //namespace ultrasonic_garage
} //namespace esphome
