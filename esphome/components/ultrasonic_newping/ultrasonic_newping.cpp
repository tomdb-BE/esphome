#include "ultrasonic_newping.h"

namespace esphome {
namespace ultrasonic_newping {

static const char *const TAG = "ultrasonic_newping.sensor";

void UltrasonicNewpingSensorComponent::setup() {
  std::string pinconf = this->trigger_pin_->dump_summary();
  ESP_LOGI(TAG, "PINCONFIG Trigger %s", pinconf.c_str());
  sonar = new NewPing(15, 5, 20);
}

void UltrasonicNewpingSensorComponent::update() {
  std::string pinconf = this->trigger_pin_->dump_summary();
  ESP_LOGI(TAG, "PINCONFIG Trigger %s", pinconf.c_str());
}

}  // namespace ultrasonic_newping
}  // namespace esphome
