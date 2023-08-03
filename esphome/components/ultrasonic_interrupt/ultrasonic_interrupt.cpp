#include "ultrasonic_interrupt.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_interrupt {

static const char *const TAG = "ultrasonic_interrupt.sensor";

void UltrasonicInterruptSensorComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Ultrasonic Sensor...");
  this->trigger_pin_->setup();
  this->trigger_pin_->digital_write(false);
  this->echo_pin_->setup();
  echo_isr_ = echo_pin_->to_isr();
  this->echo_pin_->attach_interrupt(&UltrasonicInterruptSensorComponent::interrupt_echo_callback_, this, gpio::INTERRUPT_ANY_EDGE);  
}

void UltrasonicInterruptSensorComponent::update() {
  if (this->sleeping_)
    return;

  if (!this->pulse_start_us_) {
    this->send_trigger_pulse();
  }

  if (this->previous_distance_us_ == this->distance_us_)
    return;
  
  this->previous_distance_us_ = this->distance_us_;

  if (this->distance_us_) {
    this->distance_cm_ = this->distance_us_ / ULTRASONIC_US_TO_CM;
    const int16_t distance_cm_diff = this->previous_distance_cm_ - this->distance_cm_;
    if (std::abs(distance_cm_diff) > 5)
      this->previous_distance_cm_ = this->distance_cm_;
    this->publish_state(this->distance_cm_ );
  }
  else {
    this->distance_cm_ = 0;
    this->publish_state(NAN);
  }  
}

void UltrasonicInterruptSensorComponent::dump_config() {
  LOG_SENSOR("", "Ultrasonic Sensor", this);
  LOG_PIN("  Echo Pin: ", this->echo_pin_);
  LOG_PIN("  Trigger Pin: ", this->trigger_pin_);
  ESP_LOGCONFIG(TAG, "  Pulse time: %u µs", this->pulse_time_us_);
  ESP_LOGCONFIG(TAG, "  Timeout: %u µs", this->timeout_us_);
  LOG_UPDATE_INTERVAL(this);
}

void UltrasonicInterruptSensorComponent::send_trigger_pulse() {
  this->trigger_pin_->digital_write(true);
  delayMicroseconds(5);
  this->trigger_pin_->digital_write(false);
  this->trigger_pin_->digital_write(true);
  delayMicroseconds(this->pulse_time_us_);
  this->trigger_pin_->digital_write(false);  
}

void IRAM_ATTR UltrasonicInterruptSensorComponent::interrupt_echo_callback_(UltrasonicInterruptSensorComponent *sonar) {
  if (sonar->echo_isr_.digital_read()) {
    sonar->pulse_start_us_ = micros();
    return;
  }
  if (sonar->pulse_start_us_) {
    uint32_t result_us = micros() - sonar->pulse_start_us_;
    sonar->distance_us_ = (result_us < sonar->timeout_us_) ? result_us : 0;
    sonar->pulse_start_us_ = 0;
  }
}

}  // namespace ultrasonic_interrupt
}  // namespace esphome
