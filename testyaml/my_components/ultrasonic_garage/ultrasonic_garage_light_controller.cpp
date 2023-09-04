#include "ultrasonic_garage_light_controller.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.light_controller";

void UltrasonicGarageLightController::setup() {
  turn_off_all();
}
void UltrasonicGarageLightController::add_light_trigger(UltrasonicGarageLightControllerTrigger* light_trigger, UltrasonicGarageActionType action_type) {
  light_triggers_[action_type].push_back(light_trigger);
}

void UltrasonicGarageLightController::activate_triggers(UltrasonicGarageActionType action_type) {
  if (light_triggers_[action_type].empty())
    return;
  for (UltrasonicGarageLightControllerTrigger* light_trigger : light_triggers_[action_type]) {
    light_trigger->trigger();
  }
}

} //namespace ultrasonic_garage
} //namespace esphome