#include "sonar.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_cover {

static const char *const TAG = "sonar.sensor";

void Sonar::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Ultrasonic Sensor...");
  trigger_pin_->setup();
  trigger_pin_->digital_write(false);
  echo_pin_->setup();
  echo_isr_ = echo_pin_->to_isr();
  echo_pin_->attach_interrupt(&Sonar::interrupt_echo_callback_, this, gpio::INTERRUPT_ANY_EDGE);  
}

void Sonar::update() {
  if (sleeping_)
    return;

  if (!pulse_start_us_) {
    send_trigger_pulse();
  }

  if (previous_distance_us_ == distance_us_)
    return;
  
  previous_distance_us_ = distance_us_;

  if (distance_us_) {
    distance_cm_ = distance_us_ / ULTRASONIC_US_TO_CM;
    const int16_t distance_cm_diff = previous_distance_cm_ - distance_cm_;
    if (std::abs(distance_cm_diff) > 5)
      previous_distance_cm_ = distance_cm_;
    publish_state(distance_cm_ );
  }
  else {
    distance_cm_ = 0;
    publish_state(NAN);
  }  
}

void Sonar::dump_config() {
  LOG_SENSOR("", "Ultrasonic Sensor", this);
  LOG_PIN("  Echo Pin: ", echo_pin_);
  LOG_PIN("  Trigger Pin: ", trigger_pin_);
  ESP_LOGCONFIG(TAG, "  Pulse time: %u µs", pulse_time_us_);
  ESP_LOGCONFIG(TAG, "  Timeout: %u µs", timeout_us_);
  LOG_UPDATE_INTERVAL(this);
}

void Sonar::send_trigger_pulse() {
  trigger_pin_->digital_write(true);
  delayMicroseconds(5);
  trigger_pin_->digital_write(false);
  trigger_pin_->digital_write(true);
  delayMicroseconds(pulse_time_us_);
  trigger_pin_->digital_write(false);  
}

void IRAM_ATTR Sonar::interrupt_echo_callback_(Sonar *sonar) {
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

}  // namespace sonar_sensor
}  // namespace esphome
