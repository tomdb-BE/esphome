#include "ultrasonic_garage_light_controller.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ultrasonic_garage {

static const char *const TAG = "ultrasonicgarage.light_controller";

const Color GREEN(0, 255, 0, 255);

void UltrasonicGarageLightEffect::start() {  
  led_count_ = (mirrored_) ? this->get_addressable_()->size() / 2 : this->get_addressable_()->size();
  progress_ = 0;
  get_addressable_()->all() = Color::BLACK;
  get_addressable_()->schedule_show();
  start_extra();
}

void ScanFastLightEffect::start_extra() {  
    if (reversed_)
        led_count_--;
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

void DistanceGateLightEffect::start_extra() {
    if (!gate_)       
        stop();
}
void DistanceGateLightEffect::apply(light::AddressableLight &it, const Color &current_color) {
    Color color = GREEN;
    uint8_t color_diff;
    if (gate_->position > 0.00f) {
        gate_->position_delta = 0.00f;
        color_diff = gate_->position * 128.00f;
        color = Color(255 - color_diff, color_diff, 0, 0);
    }
    it.all() = color; 
    it.schedule_show();
}


void DistanceCarLightEffect::start_extra() {
    if (! sonar_sensor_) {
        stop();
    }    
}
void DistanceCarLightEffect::apply(light::AddressableLight &it, const Color &current_color) {
    Color color = GREEN;
    uint16_t distance = sonar_sensor_->distance_cm;    
    if (distance == 0 || distance == sonar_sensor_->previous_distance_cm)
        return;

    if (distance < sonar_sensor_->segment) {
        distance = (distance << 8) / sonar_sensor_->segment;
        color = Color(255 - distance, distance, 0, 0);
    }
    it.all() = color;  
    it.schedule_show();
}

} //namespace ultrasonic_garage
} //namespace esphome