#include <esp_timer.h>
#include "ultrasonic_garage_gate.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.gate";

void UltrasonicGarageGate::setup_gate(float travel_distance) {
  this->travel_distance_ = travel_distance;
  activate_pin_->setup();
}

void UltrasonicGarageGate::dump_config() {
  const char *const TAG = "ultrasonicgarage";
  LOG_COVER("  ", "Gate", this);
  LOG_PIN("    Activate Pin: ", this->activate_pin_);
  ESP_LOGCONFIG(TAG, "    Min. position delta: %d%%", (int) (this->min_position_delta_ * 100));
  ESP_LOGCONFIG(TAG, "    Trigger time: %dms", (int) this->trigger_time_);
  ESP_LOGCONFIG(TAG, "    Operation timeout: %ds", (int) (this->operation_timeout_ / 1000));
}

uint8_t UltrasonicGarageGate::update(float gate_distance) {
  if (gate_distance >= 0.00f && fabs(gate_distance - this->position) > this->min_position_delta_) {    
    this->position = 1.00f - (gate_distance / this->travel_distance_);
    this->publish_state();
    ESP_LOGD(TAG, "Postion: %f", this->position);
  }
  if (this->trigger_gate_)
    this->handle_gate_trigger_();
  if (this->current_operation != cover::COVER_OPERATION_CLOSING)
    this->handle_gate_operation_();
  return (uint8_t) this->current_operation;
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
    float current_position = this->position;
    ESP_LOGD(TAG, "Cover activated - Position: %f", pos);
    // If position change smaller than minimum percentage, do nothing.
    if (fabs(pos - current_position) < this->min_position_delta_)
      return;
    if (pos > current_position) {
      this->current_operation = cover::COVER_OPERATION_CLOSING;
    }
    else {
      this->current_operation = cover::COVER_OPERATION_OPENING;
    }
    this->operation_direction_ = current_operation;
    this->trigger_gate_ = true;
    // Publish new state        
    this->position = pos;    
    this->publish_state();
  }
  if (call.get_stop()) {
    this->operation_direction_ = cover::COVER_OPERATION_IDLE;
    this->trigger_gate_ = true;
    // User requested cover stop
  }
}

void UltrasonicGarageGate::handle_gate_trigger_() {
  int64_t time_now = esp_timer_get_time();
  if (!this->trigger_timer_) {    
    this->activate_pin_->digital_write(true);
    this->trigger_timer_ = time_now + this->trigger_time_;
    ESP_LOGD(TAG, "Time NOW = %d", (int) time_now);
    ESP_LOGD(TAG, "Time NOW TRIGGER = %d", (int) this->trigger_time_);    
    ESP_LOGD(TAG, "Gate switch ON: timeout = %d", (int) this->trigger_timer_);    
  }
  else if (time_now > this->trigger_timer_) {  
    this->trigger_gate_ = false;   
    this->trigger_timer_ = 0;
    this->activate_pin_->digital_write(false);    
    ESP_LOGD(TAG, "Gate switch OFF");    
    this->next_direction_ = handle_gate_direction_(this->operation_direction_, this->next_direction_);
  }  
}

cover::CoverOperation UltrasonicGarageGate::handle_gate_direction_(cover::CoverOperation current_direction, cover::CoverOperation next_direction) {
    if (current_direction == cover::COVER_OPERATION_IDLE)
      return (next_direction == cover::COVER_OPERATION_CLOSING) ? cover::COVER_OPERATION_OPENING : cover::COVER_OPERATION_CLOSING;
    if (this->reverse_required_) {
      ESP_LOGD(TAG, "Triggering the gate once more in the right direction after reversing");
      this->reverse_required_ = false; 
      this->trigger_gate_ = true;
      return current_direction;
    }
    if (current_direction != next_direction) {
      ESP_LOGD(TAG, "Stopping gate to reverse direction");
      this->reverse_required_ = true;
      this->trigger_gate_ = true;
      return next_direction;
    }
    else {      
      ESP_LOGD(TAG, "Gate action completed, gate trigger OFF");
      this->reverse_required_ = false; 
      return (current_direction == cover::COVER_OPERATION_CLOSING) ? cover::COVER_OPERATION_OPENING : cover::COVER_OPERATION_CLOSING;
    }
}

void UltrasonicGarageGate::handle_gate_operation_() {
  int64_t time_now = esp_timer_get_time();
  if (!this->operation_timer_) {
    this->operation_timer_ = time_now + this->operation_timeout_;
    // ESP_LOGD(TAG, "Gate operation timer started");
  }  
  else if (esp_timer_get_time() > this->operation_timer_) {
    this->operation_timer_ = 0;
    // ESP_LOGD(TAG, "Gate operation timeout");
  }  
}

} //namespace ultrasonic_garage
} //namespace esphome
