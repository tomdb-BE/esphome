#include "ultrasonic_garage_sonar.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.sonar";

void IRAM_ATTR handle_sleep_timer(void *arg) {
  UltrasonicGarageSonar *sonar = (UltrasonicGarageSonar*) arg;
  if (sonar->is_sleeping())
    sonar->set_sonar_enabled();  
}

void IRAM_ATTR interrupt_echo_callback(UltrasonicGarageSonar *sonar) { 
  if (sonar->read_echo_pin())
    sonar->set_pulse_start_us();
  else if (sonar->get_pulse_started()) 
    sonar->set_pulse_end_us();  
}

void UltrasonicGarageSonar::setup_sonar() {
  const uint64_t double_pulse_timeout = this->pulse_timeout_us_ * 2;
  const uint64_t min_update_interval = this->min_update_interval_;
  
  this->min_update_interval_ = (double_pulse_timeout > min_update_interval) ? double_pulse_timeout : min_update_interval;
  this->trigger_pin_->setup();
  this->trigger_pin_->digital_write(false);
  this->echo_pin_->setup();
  this->echo_isr_ = echo_pin_->to_isr();
  this->echo_pin_->attach_interrupt(&interrupt_echo_callback, this, gpio::INTERRUPT_ANY_EDGE);

  if (this->sleep_update_interval_) {
    const esp_timer_create_args_t sleep_timer_args = {          
          .callback = &handle_sleep_timer,
          .arg = (void*) this,
          .dispatch_method = ESP_TIMER_TASK,
          .name = "sleep-timer",
          .skip_unhandled_events = false          
    };
    if (esp_timer_create(&sleep_timer_args, &this->sleep_timer_) != ESP_OK) {
      ESP_LOGE(TAG, "Failed to initialize sleep timer!");
      this->mark_failed();
      return;
    };
    if (esp_timer_start_periodic(this->sleep_timer_, this->sleep_update_interval_) != ESP_OK) {
      ESP_LOGE(TAG, "Failed to start sleep timer!");
      this->mark_failed();
      return;
    };
  }

  this->enabled_ = true;
}

void UltrasonicGarageSonar::update() {  
  this->relative_distance_cm_ = -1;
  
  // No pulse sent, trigger new pulse
  if (! this->pulse_timeout_timer_us_) {    
    this->send_trigger_pulse_();
    return;
  }

  // Pulse timeout. New pulse will be sent next cycle
  if (! this->pulse_end_us_ && esp_timer_get_time() > this->pulse_timeout_timer_us_) {       
    this->pulse_timeout_timer_us_ = 0;
    this->pulse_start_us_ = 0;
    return;
  }
  // Pulse response received

  // Disable next update until sleep timer expires when in sleep mode
  this->enabled_ = !this->sleep_mode_;

  // Calculate distance and reset timers
  int32_t distance = (this->pulse_end_us_ - this->pulse_start_us_) / ULTRASONIC_US_TO_CM;
  int32_t distance_diff = this->previous_distance_cm_ - distance;
  this->pulse_start_us_ = 0;
  this->pulse_end_us_ = 0;
  
  // Ignore measurement if delta too low
  if (std::abs(distance_diff) < this->min_change_)
    return;  

  // New measurement. Publish new state.
  this->previous_distance_cm_ = this->distance_cm_;
  this->distance_cm_ = distance;    
  this->publish_state(distance);

  if (distance <= this->min_distance_)
    this->relative_distance_cm_ = 0;
  else if (distance >= this->max_distance_)
    this->relative_distance_cm_ = this->max_distance_ - this->min_distance_;
  else this->relative_distance_cm_ = distance - this->min_distance_;
}

void UltrasonicGarageSonar::send_trigger_pulse_() {
  this->trigger_pin_->digital_write(true);  
  delayMicroseconds(this->pulse_time_us_);  
  this->trigger_pin_->digital_write(false);    
  this->pulse_timeout_timer_us_ = esp_timer_get_time() + this->pulse_timeout_us_;
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
  ESP_LOGCONFIG(TAG, "    Sleep timeout: %ds", (int) this->sleep_timeout_ / 1000 / 1000);
  ESP_LOGCONFIG(TAG, "    Sleep update interval: %ds", (int) this->sleep_update_interval_ / 1000 / 1000);
}

} //namespace ultrasonic_garage
} //namespace esphome
