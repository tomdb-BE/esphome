#include "ultrasonic_garage_sonar.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.sonar";

void IRAM_ATTR interrupt_echo_callback(UltrasonicGarageSonar *sonar) { sonar->pulse_end_us = esp_timer_get_time(); }

void UltrasonicGarageSonar::setup_sonar() {
  trigger_pin_->setup();
  trigger_pin_->digital_write(false);
  echo_pin_->setup();
  echo_isr_ = echo_pin_->to_isr();
  echo_pin_->attach_interrupt(&interrupt_echo_callback, this, gpio::INTERRUPT_FALLING_EDGE);
  disabled_ = false;  
}

void UltrasonicGarageSonar::update_sensor(const int64_t time_now) {  
  if (disabled_ || sleeping_) {    
    return;
  }

  if (!pulse_start_us_) {    
    send_trigger_pulse_();
    return;
  }

  if (!pulse_end_us && time_now < pulse_timer_us_) {
    return;
  }
  else if (pulse_end_us) {
    distance_us_ = pulse_end_us - pulse_start_us_;
  }

  pulse_start_us_ = 0;
  pulse_end_us = 0;

  if (previous_distance_us_ == distance_us_)
    return;
  
  previous_distance_us_ = distance_us_;

  distance_cm = distance_us_ / ULTRASONIC_US_TO_CM;
  const int16_t distance_cmdiff = previous_distance_cm - distance_cm;
  if (std::abs(distance_cmdiff) > 5)
    previous_distance_cm = distance_cm;
  publish_state(distance_cm );
}

void UltrasonicGarageSonar::dump_config() {
  const char *const TAG = "ultrasonicgarage";
  if (is_car_) {
    LOG_SENSOR("  ", "Sonar Car", this);
  }
  else {
    LOG_SENSOR("  ", "Sonar Gate", this);
  }
  LOG_PIN("    Echo Pin: ", echo_pin_);
  LOG_PIN("    Trigger Pin: ", trigger_pin_);
  ESP_LOGCONFIG(TAG, "    Pulse time: %dµs", (int) pulse_time_us_);
  ESP_LOGCONFIG(TAG, "    Pulse Timeout: %dµs", (int) timeout_us_);
  ESP_LOGCONFIG(TAG, "    Distance Timeout: %dcs", (int) timeout_cm_);
  ESP_LOGCONFIG(TAG, "    Min. Distance: %dcm", (int) min_distance_);
  ESP_LOGCONFIG(TAG, "    Max. Distance: %dcm", (int) max_distance_);
  ESP_LOGCONFIG(TAG, "    Min. Change: %dcm", (int) min_change_);
  ESP_LOGCONFIG(TAG, "    Max. Errors: %d", (int) max_errors_);
  ESP_LOGCONFIG(TAG, "    Sleep timeout: %ds", (int) sleep_timeout_);
  ESP_LOGCONFIG(TAG, "    Sleep update interval: %ds", (int) sleep_update_interval_);
}

void UltrasonicGarageSonar::send_trigger_pulse_() {
  trigger_pin_->digital_write(true);
  delayMicroseconds(5);
  trigger_pin_->digital_write(false);
  trigger_pin_->digital_write(true);
  delayMicroseconds(pulse_time_us_);
  trigger_pin_->digital_write(false);
  pulse_start_us_ = esp_timer_get_time();
  pulse_timer_us_ = esp_timer_get_time() + pulse_timeout_us_;
}

} //namespace ultrasonic_garage
} //namespace esphome
