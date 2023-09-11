#pragma once

#ifdef USE_ESP32

#include "esphome/components/light/addressable_light.h"
#include "esphome/components/light/light_output.h"
#include "esphome/core/color.h"
#include "esphome/core/component.h"

#include <driver/gpio.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>


namespace esphome {
namespace esp32_spi_led_strip {

enum RGBOrder : uint8_t {
  ORDER_RGB,
  ORDER_RBG,
  ORDER_GRB,
  ORDER_GBR,
  ORDER_BGR,
  ORDER_BRG,
};

class ESP32SPILEDStripLightOutput : public light::AddressableLight {
 public:
  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_num_leds(uint16_t num_leds) { this->num_leds_ = num_leds; }
  void set_is_rgbw(bool is_rgbw) { this->is_rgbw_ = is_rgbw; }
  void set_clock_speed(uint32_t clock_speed) { this->spi_device_config_.clock_speed_hz = clock_speed; }
  void set_dma_channel(uint8_t spi_dma_channel) { this->spi_dma_channel_ = spi_dma_channel; }
  /// Set a maximum refresh rate in µs as some lights do not like being updated too often.
  void set_max_refresh_rate(uint32_t interval_us) { this->max_refresh_rate_ = interval_us; }
  void set_rgb_order(RGBOrder rgb_order) { this->rgb_order_ = rgb_order; }

  void clear_effect_data() override { memset(this->effect_data_, 0, this->size()); }
  float get_setup_priority() const override { return setup_priority::HARDWARE; }
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

  void setup() override;
  void write_state(light::LightState *state) override;
  void dump_config() override;

    
 protected:  
  size_t get_buffer_size_() const { return this->num_leds_ * (3 + this->is_rgbw_); }
  
  light::ESPColorView get_view_internal(int32_t index) const override;
  
  uint8_t spi_dma_channel_ = SPI_DMA_CH_AUTO;
  uint16_t* spi_dma_buffer_{nullptr};
  spi_bus_config_t spi_bus_config_ = {};
  spi_device_interface_config_t spi_device_config_ = {};  
  spi_transaction_t spi_transaction_ = {};
  spi_device_handle_t spi_handle_{nullptr};

  uint8_t *buf_{nullptr};
  uint8_t *effect_data_{nullptr};
  
  uint8_t pin_;
  uint16_t num_leds_;
  bool is_rgbw_;

  RGBOrder rgb_order_;

  uint32_t last_refresh_{0};
  optional<uint32_t> max_refresh_rate_{};  
};

}  // namespace esp32_spi_led_strip
}  // namespace esphome

#endif  // USE_ESP32
