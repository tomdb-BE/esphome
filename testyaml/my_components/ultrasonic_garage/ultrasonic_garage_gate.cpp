#include "ultrasonic_garage_gate.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace cover {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.gate";

void UltrasonicGarageGate::setup_gate() {
  ESP_LOGD(TAG, "Initializing Gate...");
  activate_pin_->setup();
  if (active_pin_)
    active_pin_->setup();
  dump_config();
}

void UltrasonicGarageGate::dump_config() {
  LOG_COVER(TAG, "UltrasonicGarageGate", this);
  LOG_PIN("  Activate Pin: ", activate_pin_);
  (active_pin_set_) ? ESP_LOGCONFIG("  ", " Active sensor: ENABLED") : ESP_LOGCONFIG("  ", "  Active sensor: DISABLED");
  ESP_LOGCONFIG("  ", "Min. position delta: %d%", uint32_t(min_position_delta_ * 100.00));
  ESP_LOGCONFIG("  ", "Trigger time:  %dms", trigger_time_);
  ESP_LOGCONFIG("  ", "Operation timeout:  %ds", operation_timeout_ / 1000);
}

void UltrasonicGarageGate::update_gate() {
  reference_timer_ = millis();
  if (trigger_timer_)
    handle_gate_trigger_();
  if (current_operation)
    handle_gate_operation_();
}


CoverTraits UltrasonicGarageGate::get_traits() {
  auto traits = cover::CoverTraits();
  traits.set_is_assumed_state(false);
  traits.set_supports_position(true);
  traits.set_supports_tilt(false);
  traits.set_supports_stop(true);
  return traits;
}

void UltrasonicGarageGate::control(const CoverCall &call) {
  // This will be called every time the user requests a state change.  
  if (call.get_position().has_value()) {
    float pos = *call.get_position();
    ESP_LOGD(TAG, "Cover activated - Position: %f", pos);
    // If position change smaller than minimum percentage, do nothing.
    if (fabs(pos - position) < min_position_delta_)
      return;
    if (pos > position) {
      current_operation = COVER_OPERATION_CLOSING;
    }
    else {
      current_operation = COVER_OPERATION_OPENING;
    }
    operation_direction_ = current_operation;
    trigger_gate();
    // Publish new state
    position = pos;    
    publish_state();
  }
  if (call.get_stop()) {
    trigger_gate();
    // User requested cover stop
  }
}

void UltrasonicGarageGate::trigger_gate() {
  if (!trigger_timer_) {    
    activate_pin_->digital_write(true);
    trigger_timer_ = millis() + trigger_time_;
    ESP_LOGD(TAG, "Gate trigger ON");
  }  
}

void UltrasonicGarageGate::handle_gate_trigger_() {
  if (reference_timer_ > trigger_timer_) {  
    trigger_timer_ = 0;  
    activate_pin_->digital_write(false);
    if (reverse_required_) {
      ESP_LOGD(TAG, "Triggering the gate once more in the right direction after reversing");      
      reverse_required_ = false;
      next_direction_ = operation_direction_;    
      trigger_gate();
    }    
    else if (operation_direction_ != next_direction_) {
      ESP_LOGD(TAG, "Stopping gate to reverse direction");
      reverse_required_ = true;
      trigger_gate();      
    }
    else {
      ESP_LOGD(TAG, "Gate action completed, gate trigger OFF");
      next_direction_ = (operation_direction_ == COVER_OPERATION_CLOSING) ? COVER_OPERATION_OPENING : COVER_OPERATION_CLOSING;
    }
  }  
}

void UltrasonicGarageGate::handle_gate_operation_() {
  if (!operation_timer_) {
    operation_timer_ = reference_timer_ + operation_timeout_;
    ESP_LOGD(TAG, "Gate operation timer started");
  }  
  else if (reference_timer_ > operation_timer_) {
    operation_timer_ = 0;
    ESP_LOGD(TAG, "Gate operation timeout");
  }  
}


} //namespace ultrasonic_garage
} //namespace cover
} //namespace esphome
