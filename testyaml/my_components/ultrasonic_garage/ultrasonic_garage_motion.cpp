#include "ultrasonic_garage_motion.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.motion";

void UltrasonicGarageMotion::setup() {
  this->motion_pin_->setup();
  this->enabled_ = true;
}

void UltrasonicGarageMotion::dump_config() {
  const char *const TAG = "ultrasonicgarage";
  LOG_BINARY_SENSOR("  ", "Motion Sensor", this);
  LOG_PIN("    Pin: ", this->motion_pin_);
  ESP_LOGCONFIG(TAG, "    Inverted: %s", this->inverted_ ? "True" : "False");
  ESP_LOGCONFIG(TAG, "    Minimum ON time: %ds", (int) this->min_on_ / 1000 / 1000);
  ESP_LOGCONFIG(TAG, "    Minimum OFF time: %ds", (int) this->min_off_ / 1000 / 1000);
}

bool UltrasonicGarageMotion::update() {
  int64_t time_now = esp_timer_get_time();
  int64_t new_state_timer = this->new_state_timer_;
  bool measurement = motion_pin_->digital_read() ^ this->inverted_;
  bool previous_measurement = this->previous_measurement_;
  int64_t min_period = (measurement) ? this->min_on_ : this->min_off_;

  if (measurement == previous_measurement) {
    if (new_state_timer && time_now > new_state_timer) {
      this->new_state_timer_ = 0;
      this->last_period_ = time_now;
      publish_state(measurement);
    }       
  }
  else if (min_period) {
    this->new_state_timer_ = time_now + min_period;
    this->previous_measurement_ = measurement;  
    measurement = previous_measurement;
  }
  else {
    publish_state(measurement);
  }

  return measurement;
}

} //namespace ultrasonic_garage
} //namespace esphome
