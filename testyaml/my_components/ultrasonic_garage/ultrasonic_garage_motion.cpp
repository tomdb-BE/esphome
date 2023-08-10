#include "ultrasonic_garage_motion.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.motion";

void UltrasonicGarageMotion::setup_motion_sensor() {
  motion_pin_->setup();
}

void UltrasonicGarageMotion::dump_config() {
  const char *const TAG = "ultrasonicgarage";
  float min_on = min_on_;
  float min_off = min_off_;
  LOG_BINARY_SENSOR("  ", "Motion Sensor", this);
  LOG_PIN("    Pin: ", motion_pin_);
  ESP_LOGCONFIG(TAG, "    Inverted: %s", inverted_ ? "True" : "False");
  ESP_LOGCONFIG(TAG, "    Minimum ON time: %.3fs", min_on / 1000);
  ESP_LOGCONFIG(TAG, "    Minimum OFF time: %.3fs", min_off / 1000);
}

void UltrasonicGarageMotion::update() { 
  bool sensor_active;
  uint32_t time_now;

  if (!detection_enabled_)
    return;
  
  time_now = millis();
  if (time_now < timeout_timer_)  
    return;
  
  sensor_active = motion_pin_->digital_read();
  
  if (inverted_)
    sensor_active = !sensor_active;

  if (sensor_active == state)
    return;
  
  if (sensor_active && min_on_)
    timeout_timer_ = time_now + min_on_;
  else if (!sensor_active && min_off_)
    timeout_timer_ = time_now + min_off_;
  else timeout_timer_ = 0;

  last_period_ = time_now;
  motion_detected_ = sensor_active;
  publish_state(motion_detected_ );
}

} //namespace ultrasonic_garage
} //namespace esphome
