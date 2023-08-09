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
  is_ready_ = true;
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
  (sonar_gate_) ? ESP_LOGCONFIG(TAG, "  Gate sonar sensor: ENABLED.") : ESP_LOGCONFIG(TAG, "  Gate sonar sensor: DISABLED.");
  (sonar_car_) ? ESP_LOGCONFIG(TAG, "  Car sonar sensor: ENABLED.") : ESP_LOGCONFIG(TAG, "  Car sonar sensor: DISABLED.");  
}

} //namespace ultrasonic_garage
} //namespace cover
} //namespace esphome