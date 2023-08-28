#include "ultrasonic_garage_light_controller.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.light_controller";

const Color GREEN(0, 255, 0, 255);

void UltrasonicGarageLightEffect::start() {  
  this->led_count_ = (mirrored_) ? this->get_addressable_()->size() / 2 : this->get_addressable_()->size();
  this->progress_ = 0;
  this->get_addressable_()->all() = Color::BLACK;
  this->get_addressable_()->schedule_show();
  this->start_extra();
}

void ScanFastLightEffect::start_extra() {  
   if (reversed_)
    this->led_count_--;
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

void DistanceGateLightEffect::apply(light::AddressableLight &it, const Color &current_color) {
    Color color = Color::BLACK;
    float sensor_value = 0.00;
    uint8_t color_diff;
    if (sensor_value < 1.00) {
      color_diff = sensor_value * 128.00;
      color = Color(255 - color_diff, color_diff, 0, 0);
    }
    else {
      color = GREEN;
    }
    it.all() = color; 
    it.schedule_show();
}

void DistanceCarLightEffect::apply(light::AddressableLight &it, const Color &current_color) {   
    it.schedule_show();
}

} //namespace ultrasonic_garage
} //namespace esphome