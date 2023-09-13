#include "ultrasonic_garage.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage";

void UltrasonicGarage::setup() {
  uint32_t gate_travel_distance = 0;

  if (this->motion_sensor_)
    this->motion_sensor_->setup();

  if (this->gate_sensor_)
    this->gate_sensor_->setup(); 

  if (this->sonar_gate_) {
    this->sonar_gate_->setup_sonar();    
    this->sonar_gate_interval_us_ = this->sonar_gate_->get_min_update_interval();
    gate_travel_distance = this->sonar_gate_->get_covered_distance();
  }

  if (this->sonar_car_) {
    this->sonar_car_->setup_sonar();
    this->sonar_car_interval_us_ = this->sonar_car_->get_min_update_interval();
    if (!this->sonar_gate_)
      this->update_sonar_type_ = 1;
  }

  if (this->gate_)
    this->gate_->setup_gate(gate_travel_distance);

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
  int32_t sonar_gate_relative_distance = -1;

  if (this->motion_sensor_)
    this->motion_sensor_->update_sensor();

  if (this->gate_sensor_)
    this->gate_sensor_->update_sensor();

  if (time_now > this->sonar_timer_us_) {   
    if (this->sonar_gate_ && this->sonar_gate_->is_enabled() && this->update_sonar_type_ == 0) {
      this->sonar_timer_us_ = time_now + this->sonar_gate_interval_us_;
      this->sonar_gate_->update();
      sonar_gate_relative_distance = sonar_gate_->get_relative_distance_cm();
      this->update_sonar_type_ = (this->sonar_car_ && this->sonar_car_->is_enabled()) ? 1 : 0;
    }
    else if (this->sonar_car_ && this->sonar_car_->is_enabled() && this->update_sonar_type_ == 1) {
      this->sonar_timer_us_ = time_now + this->sonar_car_interval_us_;
      this->sonar_car_->update();
      this->update_sonar_type_ = (this->sonar_gate_ && this->sonar_gate_->is_enabled()) ? 0 : 1;
    }
  }

  if (this->gate_)      
      this->gate_->update(sonar_gate_relative_distance);

  /*
  if (this->sonar_car_ && time_now > this->sonar_car_timer_us_) {
    if (!this->sonar_gate_ || !this->sonar_gate_->is_scanning())
      sonar_distance = this->sonar_car_->update();
    this->sonar_car_timer_us_ = time_now + this->sonar_car_interval_us_;
  }
  if (this->light_controller_) {}
  */
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