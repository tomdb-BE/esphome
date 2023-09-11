#pragma once

#ifdef USE_ESP32

#include <driver/gpio.h>
#include <driver/rmt_tx.h>
#include <driver/rmt_encoder.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>


#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/light_output.h"
#include "esphome/core/color.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace esp32_spi_led_strip_v5 {

static const uint8_t RMT_CLK_DIV = 2;

typedef struct {
  spi_host_device_t host;
  spi_device_handle_t spi;
  int dma_channel;
  spi_device_interface_config_t device_config;
  spi_bus_config_t bus_config;
} spi_settings_t;

enum RGBOrder : uint8_t {
  ORDER_RGB,
  ORDER_RBG,
  ORDER_GRB,
  ORDER_GBR,
  ORDER_BGR,
  ORDER_BRG,
};

class ESP32SPILEDStripLightOutputv5 : public light::AddressableLight {
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
  size_t get_dma_buffer_size_() const { return this->get_buffer_size_() * sizeof(uint16_t) * 2 + 50; }
  
  uint16_t* spi_dma_buffer_;
  spi_bus_config_t bus_config_ = {};
  spi_device_interface_config_t device_config_ = {};
  spi_settings_t spi_settings_ = {};
  spi_transaction_t spi_transaction_ = {};

  uint8_t *buf_{nullptr};
  uint8_t *effect_data_{nullptr};
  
  uint8_t pin_;
  uint16_t num_leds_;
  bool is_rgbw_;

  bool set_once_ = false;

  RGBOrder rgb_order_;

  uint32_t last_refresh_{0};
  optional<uint32_t> max_refresh_rate_{};
};

}  // namespace esp32_spi_led_strip_v5
}  // namespace esphome

#endif  // USE_ESP32
