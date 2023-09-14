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
  UltrasonicGarageSonar* sonar_gate = this->sonar_gate_;
  UltrasonicGarageSonar* sonar_car = this->sonar_car_;
  bool update_sonar_type = this->update_sonar_type_;
  bool motion_detected = false;
  bool gate_moving = false;
  uint8_t cover_operation;
  const int64_t  time_now = esp_timer_get_time();
  int32_t sonar_gate_relative_distance = -1;

  if (this->gate_sensor_ && this->motion_sensor_->is_enabled())
    gate_moving = this->gate_sensor_->update();

  if (this->motion_sensor_ && this->motion_sensor_->is_enabled())
    motion_detected = this->motion_sensor_->update();

  if (time_now > this->sonar_timer_us_) {   
    if (sonar_gate && sonar_gate->is_enabled() && update_sonar_type == 0) {
      if (gate_moving)
        sonar_gate->set_sleep_mode(false);
      this->sonar_timer_us_ = time_now + this->sonar_gate_interval_us_;
      sonar_gate->update();
      sonar_gate_relative_distance = sonar_gate->get_relative_distance_cm();
      this->update_sonar_type_ = (sonar_car && sonar_car->is_enabled()) ? 1 : 0;
    }
    else if (sonar_car && sonar_car->is_enabled() && update_sonar_type == 1) {
      if (motion_detected)
        sonar_car->set_sleep_mode(false);      
      this->sonar_timer_us_ = time_now + this->sonar_car_interval_us_;
      sonar_car->update();
      this->update_sonar_type_ = (sonar_gate && sonar_gate->is_enabled()) ? 0 : 1;
    }
  }

  if (this->gate_)      
      cover_operation = this->gate_->update(sonar_gate_relative_distance);

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