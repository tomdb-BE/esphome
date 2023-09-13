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
    if (!this->gate_)       
        stop();
    this->previous_position_ = 0.00f;
}
void DistanceGateLightEffect::apply(light::AddressableLight &it, const Color &current_color) {
    Color color = GREEN;
    uint8_t color_diff;
    float current_position = this->gate_->position;

    if (current_position == this->previous_position_)
      return;

    this->previous_position_ = current_position;    
    if (current_position > 0.00f) {        
        color_diff = current_position * 255.00f;
        color = Color(color_diff, 255 - color_diff, 0, 0);
    }
    it.all() = color; 
    it.schedule_show();
}


void DistanceCarLightEffect::start_extra() {
    if (!this->sonar_sensor_)
        stop();
    else this->segment_ = this->sonar_sensor_->get_effect_segment();
    this->previous_distance_ = 0;
}
void DistanceCarLightEffect::apply(light::AddressableLight &it, const Color &current_color) {
    Color color = GREEN;
    uint32_t distance = this->sonar_sensor_->get_distance_cm();
    uint32_t segment = this->segment_;
    if (distance == 0 || distance == this->previous_distance_)
        return;

    if (distance < segment) {
        distance = (distance << 8) / segment;
        color = Color(255 - distance, distance, 0, 0);
    }
    it.all() = color;  
    it.schedule_show();
}

} //namespace ultrasonic_garage
} //namespace esphome