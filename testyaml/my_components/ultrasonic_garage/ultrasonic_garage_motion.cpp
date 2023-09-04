#include "ultrasonic_garage_motion.h"
//#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.motion";

void UltrasonicGarageMotion::setup_motion_sensor() {
  motion_pin_->setup();
  const esp_timer_create_args_t motion_sensor_timer_args = {          
          .callback = &motion_sensor_timer_callback,
          .arg = (void*) this,
          .dispatch_method = ESP_TIMER_TASK,
          .name = "motion_sensor_timer",
          .skip_unhandled_events = false          
  };  
  ESP_ERROR_CHECK(esp_timer_create(&motion_sensor_timer_args, &motion_sensor_timer_));
  ESP_ERROR_CHECK(esp_timer_start_periodic(motion_sensor_timer_, min_on_));
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

void UltrasonicGarageMotion::update_sensor(const int64_t time_now) { 
  if (!sleeping)
    ESP_LOGD(TAG, "NOT Sleeping");

  if (detection_disabled_ || sleeping)
    return;
  
  bool sensor_active = motion_pin_->digital_read();

  if (inverted_)
    sensor_active = !sensor_active;

  if (sensor_active)
    ESP_LOGD(TAG, "ACTIVE");
  else
    ESP_LOGD(TAG, "NOT ACTIVE");

  //if (sensor_active == state)
    //return;  

  sleeping = true;

  if (sensor_active)
    esp_timer_restart(motion_sensor_timer_, min_off_);
  else esp_timer_restart(motion_sensor_timer_, min_on_);

  publish_state(sensor_active);

}

} //namespace ultrasonic_garage
} //namespace esphome
