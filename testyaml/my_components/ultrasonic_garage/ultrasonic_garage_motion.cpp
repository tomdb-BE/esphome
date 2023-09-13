#include "ultrasonic_garage_motion.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.motion";

void IRAM_ATTR motion_sensor_timer_callback (void* arg) { 
  UltrasonicGarageMotion *motion_sensor = (UltrasonicGarageMotion*) (arg); 
  motion_sensor->set_sleeping(false);
}

void UltrasonicGarageMotion::setup_motion_sensor() {
  this->motion_pin_->setup();
  const esp_timer_create_args_t motion_sensor_timer_args = {          
          .callback = &motion_sensor_timer_callback,
          .arg = (void*) this,
          .dispatch_method = ESP_TIMER_TASK,
          .name = "motion_sensor_timer",
          .skip_unhandled_events = false          
  };  
  if (esp_timer_create(&motion_sensor_timer_args, &this->motion_sensor_timer_) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize motion sensor timer!");
    this->mark_failed();
    return;
  };
  if (esp_timer_start_periodic(this->motion_sensor_timer_, this->min_on_) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start motion sensor timer!");
    this->mark_failed();
    return;
  };  
}

void UltrasonicGarageMotion::dump_config() {
  const char *const TAG = "ultrasonicgarage";
  LOG_BINARY_SENSOR("  ", "Motion Sensor", this);
  LOG_PIN("    Pin: ", this->motion_pin_);
  ESP_LOGCONFIG(TAG, "    Inverted: %s", this->inverted_ ? "True" : "False");
  ESP_LOGCONFIG(TAG, "    Minimum ON time: %ds", (int) this->min_on_ / 1000 / 1000);
  ESP_LOGCONFIG(TAG, "    Minimum OFF time: %ds", (int) this->min_off_ / 1000 / 1000);
}

void UltrasonicGarageMotion::update_sensor() { 
  if (!this->sleeping_)
    ESP_LOGD(TAG, "NOT Sleeping");

  if (this->detection_disabled_ || this->sleeping_)
    return;
  
  bool sensor_active = motion_pin_->digital_read();

  if (this->inverted_)
    sensor_active = !sensor_active;

  if (sensor_active)
    ESP_LOGD(TAG, "ACTIVE");
  else
    ESP_LOGD(TAG, "NOT ACTIVE");

  this->sleeping_ = true;

  if (sensor_active)
    esp_timer_restart(this->motion_sensor_timer_, min_off_);
  else esp_timer_restart(this->motion_sensor_timer_, min_on_);

  publish_state(sensor_active);

}

} //namespace ultrasonic_garage
} //namespace esphome
