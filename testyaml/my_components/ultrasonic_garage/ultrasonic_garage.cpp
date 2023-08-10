#include "ultrasonic_garage.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace cover {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage";

void UltrasonicGarage::setup() {  
  ESP_LOGD(TAG, "Setting up Ultrasonic Garage component...");
  if (gate_)
    gate_->setup_gate();  
  if (sonar_gate_)
    sonar_gate_->setup_sonar();
  if (sonar_car_)
    sonar_car_->setup_sonar();
}

void UltrasonicGarage::update() {
  if (gate_)
    gate_->update_gate();  
  if (sonar_gate_)
    sonar_gate_->update();
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
}

} //namespace ultrasonic_garage
} //namespace cover
} //namespace esphome