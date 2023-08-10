#include "ultrasonic_garage_sonar.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace cover {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage";

void UltrasonicGarageSonar::setup_sonar() {
  ESP_LOGCONFIG(TAG, "Initializing Ultrasonic Sensor...");
  trigger_pin_->setup();
  trigger_pin_->digital_write(false);
  echo_pin_->setup();
  echo_isr_ = echo_pin_->to_isr();
  echo_pin_->attach_interrupt(&UltrasonicGarageSonar::interrupt_echo_callback_, &distance_us_, gpio::INTERRUPT_FALLING_EDGE);
  dump_config();
}

void UltrasonicGarageSonar::update() {
  if (sleeping_)
    return;

  if (!pulse_start_us_) {
    send_trigger_pulse();
    return;
  }

  if (!pulse_end_us_)
    return;
  
  distance_us_ = pulse_end_us_ - pulse_start_us_;
  pulse_start_us_ = 0;
  pulse_end_us_ = 0;

  if (previous_distance_us_ == distance_us_)
    return;
  
  previous_distance_us_ = distance_us_;

  distance_cm_ = distance_us_ / ULTRASONIC_US_TO_CM;
  const int16_t distance_cm_diff = previous_distance_cm_ - distance_cm_;
  if (std::abs(distance_cm_diff) > 5)
    previous_distance_cm_ = distance_cm_;
  publish_state(distance_cm_ );
}

void UltrasonicGarageSonar::dump_config() {
  if (is_car_) {
    LOG_SENSOR("  ", "Sonar Car", this);
  }
  else {
    LOG_SENSOR("  ", "Sonar Gate", this);
  }
  LOG_PIN("    Echo Pin: ", echo_pin_);
  LOG_PIN("    Trigger Pin: ", trigger_pin_);
  ESP_LOGCONFIG(TAG, "    Pulse time: %dµs", pulse_time_us_);
  ESP_LOGCONFIG(TAG, "    Pulse Timeout: %dm", timeout_us_);
  ESP_LOGCONFIG(TAG, "    Min. Distance: %dm", min_distance_);
  ESP_LOGCONFIG(TAG, "    Max. Distance: %dm", max_distance_);
  ESP_LOGCONFIG(TAG, "    Min. Change: %dm", min_change_);
  ESP_LOGCONFIG(TAG, "    Max. Errors: %dm", max_errors_);
  ESP_LOGCONFIG(TAG, "    Min. Change: %dm", min_change_);
  ESP_LOGCONFIG(TAG, "    Sleep timeout: %ds", sleep_timeout_);
  ESP_LOGCONFIG(TAG, "    Sleep update interval: %dm", sleep_update_interval_);
}

void UltrasonicGarageSonar::send_trigger_pulse() {
  trigger_pin_->digital_write(true);
  delayMicroseconds(5);
  trigger_pin_->digital_write(false);
  trigger_pin_->digital_write(true);
  delayMicroseconds(pulse_time_us_);
  trigger_pin_->digital_write(false);
  pulse_start_us_ = micros();
}

void IRAM_ATTR UltrasonicGarageSonar::interrupt_echo_callback_(uint32_t *interrupt_triggered_var) {
  *interrupt_triggered_var = micros();
}

} //namespace ultrasonic_garage
} //namespace cover
} //namespace esphome
