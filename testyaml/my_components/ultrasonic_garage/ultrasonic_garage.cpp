#include "ultrasonic_garage.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage";

void UltrasonicGarage::setup() {  
  if (this->motion_sensor_)
    this->motion_sensor_->setup();
  if (this->gate_sensor_)
    this->gate_sensor_->setup();        
  if (this->sonar_gate_)
    this->sonar_gate_->setup_sonar();      
  if (this->sonar_car_)
    this->sonar_car_->setup_sonar();
  if (this->gate_)
    this->gate_->setup_gate();
  if (this->light_controller_) {
    this->light_controller_->setup();
    if (this->sonar_gate_)
      this->light_controller_->activate_triggers(DISTANCE_GATE);
    if (this->sonar_car_)
      this->light_controller_->activate_triggers(DISTANCE_CAR);
  }
}

void UltrasonicGarage::update() {
  const int64_t  time_now = esp_timer_get_time();
  if (this->motion_sensor_) {}
    this->motion_sensor_->update_sensor(time_now);
  if (this->gate_sensor_) {}
    this->gate_sensor_->update_sensor(time_now);    
  if (this->sonar_gate_ && time_now > this->sonar_gate_timer_us_) {
    if (!this->sonar_car_ || !this->sonar_car_->is_scanning())
      this->sonar_gate_->update();
    this->sonar_gate_timer_us_ = time_now + sonar_gate_interval_us_;
    if (this->gate_) {}
      this->gate_->update_gate(time_now);
  }
  if (this->sonar_car_ && time_now > this->sonar_car_timer_us_) {
    if (!this->sonar_gate_ || !this->sonar_gate_->is_scanning())
    this->sonar_car_->update();
    this->sonar_car_timer_us_ = time_now + this->sonar_car_interval_us_;
  }
  if (this->light_controller_) {
  }
}

void UltrasonicGarage::dump_config() {
  ESP_LOGCONFIG(TAG, "Ultrasonic Garage:");
  LOG_UPDATE_INTERVAL(this);
  if (this->gate_)
    this->gate_->dump_config();    
  if (this->sonar_gate_)
    this->sonar_gate_->dump_config();
  if (this->sonar_car_)
    this->sonar_car_->dump_config();
  if (this->gate_sensor_)
    this->gate_sensor_->dump_config();        
  if (this->motion_sensor_)
    this->motion_sensor_->dump_config();
}

} //namespace ultrasonic_garage
} //namespace esphome