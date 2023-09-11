#include "ultrasonic_garage_sonar.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.sonar";

void IRAM_ATTR interrupt_echo_callback(UltrasonicGarageSonar *sonar) { sonar->set_pulse_end_us(esp_timer_get_time()); }

void UltrasonicGarageSonar::setup_sonar() {
  this->trigger_pin_->setup();
  this->trigger_pin_->digital_write(false);
  this->echo_pin_->setup();
  this->echo_isr_ = echo_pin_->to_isr();
  this->echo_pin_->attach_interrupt(&interrupt_echo_callback, this, gpio::INTERRUPT_FALLING_EDGE);
  this->disabled_ = false;  
}

void UltrasonicGarageSonar::update() {     
  if (this->disabled_ || this->sleeping_)
    return;
  
  if (! this->pulse_start_us_) {    
    this->send_trigger_pulse_();
    return;
  }

  if (! this->pulse_end_us_ && esp_timer_get_time() > this->pulse_timeout_timer_us_) {        
    this->pulse_start_us_ = 0;
    return;
  }
   
  uint32_t distance = (this->pulse_end_us_ - this->pulse_start_us_) / ULTRASONIC_US_TO_CM;
  int32_t disance_diff = this->previous_distance_cm_ - distance;
  this->pulse_start_us_ = 0;
  this->pulse_end_us_ = 0;
  
  if (std::abs(disance_diff) > this->min_change_) {
    this->previous_distance_cm_ = this->distance_cm_;
    this->distance_cm_ = distance;    
    this->publish_state(distance);
  }
}

void UltrasonicGarageSonar::send_trigger_pulse_() {
  this->trigger_pin_->digital_write(true);
  delayMicroseconds(5);
  this->trigger_pin_->digital_write(false);
  delayMicroseconds(5);
  this->trigger_pin_->digital_write(true);  
  delayMicroseconds(this->pulse_time_us_);  
  this->trigger_pin_->digital_write(false);  
  this->pulse_start_us_ = esp_timer_get_time();
  this->pulse_timeout_timer_us_ = this->pulse_start_us_ + this->pulse_timeout_us_;
}

void UltrasonicGarageSonar::dump_config() {
  const char *const TAG = "ultrasonicgarage";
  if (this->is_car_) {
    LOG_SENSOR("  ", "Sonar Car", this);
  }
  else {
    LOG_SENSOR("  ", "Sonar Gate", this);
  }
  LOG_PIN("    Echo Pin: ", echo_pin_);
  LOG_PIN("    Trigger Pin: ", trigger_pin_);
  ESP_LOGCONFIG(TAG, "    Pulse time: %dµs", (int) this->pulse_time_us_);
  ESP_LOGCONFIG(TAG, "    Pulse Timeout: %dµs", (int) this->pulse_timeout_us_);
  ESP_LOGCONFIG(TAG, "    Distance Timeout: %dcs", (int) this->timeout_cm_);
  ESP_LOGCONFIG(TAG, "    Min. Distance: %dcm", (int) this->min_distance_);
  ESP_LOGCONFIG(TAG, "    Max. Distance: %dcm", (int) this->max_distance_);
  ESP_LOGCONFIG(TAG, "    Min. Change: %dcm", (int) this->min_change_);
  ESP_LOGCONFIG(TAG, "    Max. Errors: %d", (int) this->max_errors_);
  ESP_LOGCONFIG(TAG, "    Sleep timeout: %ds", (int) this->sleep_timeout_);
  ESP_LOGCONFIG(TAG, "    Sleep update interval: %ds", (int) this->sleep_update_interval_);
}

} //namespace ultrasonic_garage
} //namespace esphome
