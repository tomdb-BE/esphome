#ifdef USE_ESP32

#include <cinttypes>
#include "spi_led_strip_v5.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <esp_attr.h>

namespace esphome {
namespace esp32_spi_led_strip_v5 {

static const char *const TAG = "esp32_spi_led_strip";

static const uint16_t bit_mask_table[16] = {
  0x8888,
  0x8C88,
  0xC888,
  0xCC88,
  0x888C,
  0x8C8C,
  0xC88C,
  0xCC8C,
  0x88C8,
  0x8CC8,
  0xC8C8,
  0xCCC8,
  0x88CC,
  0x8CCC,
  0xC8CC,
  0xCCCC
};

void ESP32SPILEDStripLightOutputv5::setup() {
  ESP_LOGCONFIG(TAG, "Setting up ESP32 LED Strip - ESP-IDF > 5.x...");

  size_t buffer_size = this->get_buffer_size_();
  size_t dma_buffer_size = this->get_dma_buffer_size_();
  
  ExternalRAMAllocator<uint8_t> allocator(ExternalRAMAllocator<uint8_t>::ALLOW_FAILURE);
  this->buf_ = allocator.allocate(buffer_size);
  if (this->buf_ == nullptr) {
    ESP_LOGE(TAG, "Cannot allocate LED buffer!");
    this->mark_failed();
    return;
  }

  this->effect_data_ = allocator.allocate(this->num_leds_);
  if (this->effect_data_ == nullptr) {
    ESP_LOGE(TAG, "Cannot allocate effect data!");
    this->mark_failed();
    return;
  }

  this->spi_dma_buffer_ = (uint16_t*) heap_caps_malloc(dma_buffer_size, MALLOC_CAP_DMA);
  memset(this->spi_dma_buffer_, 0, dma_buffer_size);

  this->bus_config_.mosi_io_num = this->pin_;
  this->bus_config_.miso_io_num = -1;
  this->bus_config_.sclk_io_num = -1;
  this->bus_config_.quadwp_io_num = -1;
  this->bus_config_.quadhd_io_num = -1;
  this->bus_config_.max_transfer_sz = dma_buffer_size;
  this->device_config_.mode = 0;
  this->device_config_.spics_io_num = -1;
  this->device_config_.clock_speed_hz = 3.2 * 1000 * 1000;
  this->device_config_.queue_size = 1;
  this->device_config_.command_bits = 0;
  this->device_config_.address_bits = 0;

  this->spi_settings_.host = HSPI_HOST;
  this->spi_settings_.dma_channel = SPI_DMA_CH_AUTO;
  this->spi_settings_.device_config = this->device_config_;
  this->spi_settings_.bus_config = this->bus_config_;

  this->spi_transaction_.length = dma_buffer_size * 8;
  this->spi_transaction_.tx_buffer = this->spi_dma_buffer_;

  if (spi_bus_initialize(this->spi_settings_.host, &this->spi_settings_.bus_config, this->spi_settings_.dma_channel) != ESP_OK) {
   ESP_LOGE(TAG, "Cannot initialize SPI bus!");
    this->mark_failed();
    return;
  }  

  if (spi_bus_add_device(this->spi_settings_.host, &this->spi_settings_.device_config, &this->spi_settings_.spi) != ESP_OK) {
   ESP_LOGE(TAG, "Cannot add SPI device!");
    this->mark_failed();
    return;
  } 
}

void ESP32SPILEDStripLightOutputv5::set_led_params(uint32_t bit0_high, uint32_t bit0_low, uint32_t bit1_high,
                                                 uint32_t bit1_low) {

  return;                                                  

}

void ESP32SPILEDStripLightOutputv5::write_state(light::LightState *state) {
  // protect from refreshing too often  
  uint32_t now = micros();
  if (*this->max_refresh_rate_ != 0 && (now - this->last_refresh_) < *this->max_refresh_rate_) {
    // try again next loop iteration, so that this change won't get lost
    this->schedule_show();
    return;
  }

  this->last_refresh_ = now;
  this->mark_shown_();

  ESP_LOGVV(TAG, "Writing RGB values to SPI bus...");  
  memset(this->spi_dma_buffer_, 0, this->get_dma_buffer_size_());
  size_t buffer_size = this->get_buffer_size_();
  uint32_t led_color_byte = 0;
  uint32_t buffer_index = 0;
  for (uint32_t i = 0; i < buffer_size; i++) {      
    led_color_byte = this->buf_[i];    
    this->spi_dma_buffer_[buffer_index++] = bit_mask_table[0x0f & (led_color_byte >> 4)];
    this->spi_dma_buffer_[buffer_index++] = bit_mask_table[0x0f & (led_color_byte)];    
  }

  
  if (spi_device_transmit(this->spi_settings_.spi, &this->spi_transaction_) != ESP_OK) {
    ESP_LOGE(TAG, "SPI TX error");
    this->status_set_warning();
    return;
  }    

  this->status_clear_warning();
}

light::ESPColorView ESP32SPILEDStripLightOutputv5::get_view_internal(int32_t index) const {
  int32_t r = 0, g = 0, b = 0;
  switch (this->rgb_order_) {
    case ORDER_RGB:
      r = 0;
      g = 1;
      b = 2;
      break;
    case ORDER_RBG:
      r = 0;
      g = 2;
      b = 1;
      break;
    case ORDER_GRB:
      r = 1;
      g = 0;
      b = 2;
      break;
    case ORDER_GBR:
      r = 2;
      g = 0;
      b = 1;
      break;
    case ORDER_BGR:
      r = 2;
      g = 1;
      b = 0;
      break;
    case ORDER_BRG:
      r = 1;
      g = 2;
      b = 0;
      break;
  }
  uint8_t multiplier = this->is_rgbw_ ? 4 : 3;
  return {this->buf_ + (index * multiplier) + r,
          this->buf_ + (index * multiplier) + g,
          this->buf_ + (index * multiplier) + b,
          this->is_rgbw_ ? this->buf_ + (index * multiplier) + 3 : nullptr,
          &this->effect_data_[index],
          &this->correction_};
}

void ESP32SPILEDStripLightOutputv5::dump_config() {  
  ESP_LOGCONFIG(TAG, "ESP32 RMT LED Strip - ESP-IDF > v5:");
  ESP_LOGCONFIG(TAG, "  Pin: %u", this->pin_);  
  //ESP_LOGCONFIG(TAG, "  Channel: %d", this->channel_->channel_id);
  const char *rgb_order;
  switch (this->rgb_order_) {
    case ORDER_RGB:
      rgb_order = "RGB";
      break;
    case ORDER_RBG:
      rgb_order = "RBG";
      break;
    case ORDER_GRB:
      rgb_order = "GRB";
      break;
    case ORDER_GBR:
      rgb_order = "GBR";
      break;
    case ORDER_BGR:
      rgb_order = "BGR";
      break;
    case ORDER_BRG:
      rgb_order = "BRG";
      break;
    default:
      rgb_order = "UNKNOWN";
      break;
  }
  ESP_LOGCONFIG(TAG, "  RGB Order: %s", rgb_order);
  ESP_LOGCONFIG(TAG, "  Max refresh rate: %" PRIu32, *this->max_refresh_rate_);
  ESP_LOGCONFIG(TAG, "  Number of LEDs: %u", this->num_leds_);
}

float ESP32SPILEDStripLightOutputv5::get_setup_priority() const { return setup_priority::HARDWARE; }

}  // namespace esp32_spi_led_strip_v5
}  // namespace esphome

#endif  // USE_ESP32
