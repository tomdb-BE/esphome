#pragma once

#ifdef USE_ESP32

#include <driver/gpio.h>
#include <driver/rmt_tx.h>
#include <driver/rmt_encoder.h>


#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/light_output.h"
#include "esphome/core/color.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace esp32_rmt_led_strip_v5 {

static const uint8_t RMT_CLK_DIV = 2;

typedef struct {
    uint32_t resolution; /*!< Encoder resolution, in Hz */
} led_strip_encoder_config_t;

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;

enum RGBOrder : uint8_t {
  ORDER_RGB,
  ORDER_RBG,
  ORDER_GRB,
  ORDER_GBR,
  ORDER_BGR,
  ORDER_BRG,
};

class ESP32RMTLEDStripLightOutputv5 : public light::AddressableLight {
 public:
  void setup() override;
  void write_state(light::LightState *state) override;
  float get_setup_priority() const override;

  int32_t size() const override { return this->num_leds_; }
  light::LightTraits get_traits() override {
    auto traits = light::LightTraits();
    if (this->is_rgbw_) {
      traits.set_supported_color_modes({light::ColorMode::RGB_WHITE, light::ColorMode::WHITE});
    } else {
      traits.set_supported_color_modes({light::ColorMode::RGB});
    }
    return traits;
  }

  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_num_leds(uint16_t num_leds) { this->num_leds_ = num_leds; }
  void set_is_rgbw(bool is_rgbw) { this->is_rgbw_ = is_rgbw; }

  /// Set a maximum refresh rate in µs as some lights do not like being updated too often.
  void set_max_refresh_rate(uint32_t interval_us) { this->max_refresh_rate_ = interval_us; }

  void set_led_params(uint32_t bit0_high, uint32_t bit0_low, uint32_t bit1_high, uint32_t bit1_low);

  void set_rgb_order(RGBOrder rgb_order) { this->rgb_order_ = rgb_order; }
  
  void clear_effect_data() override {
    for (int i = 0; i < this->size(); i++)
      this->effect_data_[i] = 0;
  }

  void dump_config() override;
    
 protected:
  light::ESPColorView get_view_internal(int32_t index) const override;
  size_t get_buffer_size_() const { return this->num_leds_ * (3 + this->is_rgbw_); }
  
  // Encoder
  const uint16_t reset_ticks_ = APB_CLK_FREQ / RMT_CLK_DIV / 1000000 * 50 / 2; // reset code duration defaults to 50us
  rmt_copy_encoder_config_t copy_encoder_config = {};
  rmt_bytes_encoder_config_t bytes_encoder_config_ = {};
  rmt_led_strip_encoder_t led_strip_encoder_;
  rmt_encoder_handle_t led_strip_encoder_handle_{nullptr}; 
  static size_t rmt_encode_led_strip_(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state);
  static esp_err_t rmt_del_led_strip_encoder_(rmt_encoder_t *encoder);
  static esp_err_t rmt_led_strip_encoder_reset_(rmt_encoder_t *encoder);
  esp_err_t rmt_setup_led_strip_encoder_(rmt_encoder_handle_t *ret_encoder_handle);

  rmt_tx_channel_config_t channel_config_;  
  rmt_transmit_config_t transmit_config_;
  rmt_channel_handle_t channel_;
  
  uint8_t *buf_{nullptr};
  uint8_t *effect_data_{nullptr};
  
  uint8_t pin_;
  uint16_t num_leds_;
  bool is_rgbw_;

  RGBOrder rgb_order_;

  uint32_t last_refresh_{0};
  optional<uint32_t> max_refresh_rate_{};
};

}  // namespace esp32_rmt_led_strip_v5
}  // namespace esphome

#endif  // USE_ESP32
