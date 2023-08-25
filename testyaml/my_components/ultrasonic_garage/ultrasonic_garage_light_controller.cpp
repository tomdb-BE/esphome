#include "ultrasonic_garage_light_controller.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.light_controller";

void UltrasonicGarageLightController::activate_triggers(UltrasonicGarageActionType action_type) {
  if (light_triggers_[action_type].empty())
    return;
  for (UltrasonicGarageLightControllerTrigger* light_trigger : light_triggers_[action_type]) {
    light_trigger->trigger();
  }
}

void ScanFastLightEffect::start() {  
  this->led_count_ = this->get_addressable_()->size();
  this->progress_ = 0;
  if (mirrored_)
    this->led_count_ = this->led_count_ / 2;
  if (reversed_)
    this->led_count_--;    
  this->get_addressable_()->all() = Color::BLACK;
  this->get_addressable_()->schedule_show();  
}
void ScanFastLightEffect::apply(light::AddressableLight &it, const Color &current_color) {    
    if (progress_ == led_count_ || progress_ == 0) {
      it.all() = Color::BLACK;
      progress_ = (reversed_) ? led_count_ : 0;
    }
    else {
      (reversed_) ? it[progress_ + 1] = Color::BLACK : it[progress_ - 1] = Color::BLACK;
    }
    it[progress_] = current_color;
    (reversed_) ? progress_-- : progress_++;
    it.schedule_show();
}

void FillFastLightEffect::start() {  
  this->led_count_ = (mirrored_) ? this->get_addressable_()->size() : this->get_addressable_()->size() / 2;
  this->progress_ = 0;
  this->get_addressable_()->all() = Color::BLACK;
  this->get_addressable_()->schedule_show();
}
void FillFastLightEffect::apply(light::AddressableLight &it, const Color &current_color) {    
    if (progress_ == led_count_ || progress_ == 0) {
      it.all() = Color::BLACK;
      progress_ = (reversed_) ? led_count_ : 0;
    }
    if (reversed_) {
      it.range(progress_, led_count_) = current_color;
      progress_--;
    }
    else {
      it.range(0, progress_) = current_color;      
      progress_++;
    }
    it.schedule_show();
}

void GateDistanceLightEffect::start() {
  if (!this->sonar_sensor_ && distance_gate_sonar)
    this->sonar_sensor_ = distance_gate_sonar;
  this->led_count_ = (mirrored_) ? this->get_addressable_()->size() : this->get_addressable_()->size() / 2;
  this->get_addressable_()->all() = Color::BLACK;
  this->get_addressable_()->schedule_show();
}
void GateDistanceLightEffect::apply(light::AddressableLight &it, const Color &current_color) {    
    it.schedule_show();
}

void CarDistanceLightEffect::start() {
  this->led_count_ = (mirrored_) ? this->get_addressable_()->size() : this->get_addressable_()->size() / 2;
  this->get_addressable_()->all() = Color::BLACK;
  this->get_addressable_()->schedule_show();
}
void CarDistanceLightEffect::apply(light::AddressableLight &it, const Color &current_color) {    
    it.schedule_show();
}

} //namespace ultrasonic_garage
} //namespace esphome