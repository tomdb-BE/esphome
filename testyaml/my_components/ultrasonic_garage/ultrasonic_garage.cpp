#include "ultrasonic_garage.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage";

void UltrasonicGarage::setup() {  
  if (motion_sensor_)
    motion_sensor_->setup();
  if (gate_sensor_)
    gate_sensor_->setup();        
  if (sonar_gate_)
    sonar_gate_->setup_sonar();      
  if (sonar_car_)
    sonar_car_->setup_sonar();
  if (gate_)
    gate_->setup_gate();
  if (light_controller_) {
    light_controller_->setup();
    light_controller_->activate_triggers(DISTANCE_GATE);
    light_controller_->activate_triggers(DISTANCE_CAR);
  }
}

void UltrasonicGarage::update() {
  const int64_t  time_now = esp_timer_get_time();
  if (motion_sensor_) {}
    motion_sensor_->update_sensor(time_now);
  if (gate_sensor_) {}
    gate_sensor_->update_sensor(time_now);    
  if (sonar_gate_ && time_now > sonar_gate_timer_us_) {
    sonar_gate_->update_sensor(time_now);
    sonar_gate_timer_us_ = time_now + sonar_gate_interval_us_;
    if (gate_) {}
      gate_->update_gate(time_now);
  }
  if (sonar_car_ && time_now > sonar_car_timer_us_) {
    sonar_car_->update_sensor(time_now);
    sonar_car_timer_us_ = time_now + sonar_car_interval_us_;
  }
  if (light_controller_) {
    /*
    static uint32_t test = millis() + 2000;
    static bool test_effect_type = false;
    if (millis() > test) {
      test = millis() + 2000;      
      (test_effect_type) ? light_controller_->activate_triggers(DISTANCE_CAR) : light_controller_->activate_triggers(MOTION);
      test_effect_type = !test_effect_type;    
    }
    */
  }
}

void UltrasonicGarage::dump_config() {
  ESP_LOGCONFIG(TAG, "Ultrasonic Garage:");
  LOG_UPDATE_INTERVAL(this);
  if (gate_)
    gate_->dump_config();    
  if (sonar_gate_)
    sonar_gate_->dump_config();
  if (sonar_car_)
    sonar_car_->dump_config();
  if (gate_sensor_)
    gate_sensor_->dump_config();        
  if (motion_sensor_)
    motion_sensor_->dump_config();
}

} //namespace ultrasonic_garage
} //namespace esphome