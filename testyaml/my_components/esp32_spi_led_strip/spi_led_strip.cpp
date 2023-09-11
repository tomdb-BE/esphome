#ifdef USE_ESP32

#include <cinttypes>
#include "spi_led_strip.h"
#include "esphome/core/log.h"

#include <esp_attr.h>

namespace esphome {
namespace esp32_spi_led_strip {

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

void ESP32SPILEDStripLightOutput::setup() {
  ESP_LOGCONFIG(TAG, "Setting up ESP32 SPI LED Strip...");

  const size_t buffer_size = this->get_buffer_size_();
  const size_t dma_buffer_size = buffer_size * sizeof(uint16_t) * 2 + 50;
  const size_t number_of_leds = this->num_leds_;
  ExternalRAMAllocator<uint8_t> allocator(ExternalRAMAllocator<uint8_t>::ALLOW_FAILURE);

  // Allocating led buffer memory and zero at success
  this->buf_ = allocator.allocate(buffer_size);
  if (this->buf_ == nullptr) {
    ESP_LOGE(TAG, "Cannot allocate LED buffer!");
    this->mark_failed();
    return;
  }
  else {
    memset(this->buf_, 0, buffer_size);
  }

  // Allocating effect data memory and zero at success
  this->effect_data_ = allocator.allocate(number_of_leds);
  if (this->effect_data_ == nullptr) {
    ESP_LOGE(TAG, "Cannot allocate effect data!");
    this->mark_failed();
    return;
  }
  else {
    memset(this->effect_data_, 0, number_of_leds);
  }

  // Allocating SPI buffer DMA memory and zero at success
  this->spi_dma_buffer_ = (uint16_t*) heap_caps_malloc(dma_buffer_size, MALLOC_CAP_DMA);
    if (this->spi_dma_buffer_ == nullptr) {
    ESP_LOGE(TAG, "Cannot allocate SPI DMA buffer!");
    this->mark_failed();
    return;
  }
  else {
    memset(this->spi_dma_buffer_, 0, dma_buffer_size);
  }

  // Configuring and initializing SPI bus
  this->spi_bus_config_.mosi_io_num = this->pin_;
  this->spi_bus_config_.miso_io_num = -1;
  this->spi_bus_config_.sclk_io_num = -1;
  this->spi_bus_config_.quadwp_io_num = -1;
  this->spi_bus_config_.quadhd_io_num = -1;
  this->spi_bus_config_.max_transfer_sz = dma_buffer_size;

  if (spi_bus_initialize(HSPI_HOST, &this->spi_bus_config_, this->spi_dma_channel_) != ESP_OK) {
    ESP_LOGE(TAG, "Cannot initialize SPI bus!");
    this->mark_failed();
    return;
  }

  // Configuring and adding SPI device
  this->spi_device_config_.mode = 0;
  this->spi_device_config_.spics_io_num = -1;
  this->spi_device_config_.queue_size = 1;
  this->spi_device_config_.command_bits = 0;
  this->spi_device_config_.address_bits = 0;

  if (spi_bus_add_device(HSPI_HOST, &this->spi_device_config_, &this->spi_handle_) != ESP_OK) {
    ESP_LOGE(TAG, "Cannot add SPI device!");
    this->mark_failed();
    return;
  }

  // Setting up SPI transaction
  this->spi_transaction_.length = dma_buffer_size * 8;
  this->spi_transaction_.tx_buffer = this->spi_dma_buffer_;

}

void ESP32SPILEDStripLightOutput::write_state(light::LightState *state) {
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
  
  // Setting DMA buffer
  const size_t buffer_size = this->get_buffer_size_();
  uint16_t led_color_byte = 0;
  uint16_t buffer_index = 0;
  for (uint16_t i = 0; i < buffer_size; i++) {      
    led_color_byte = this->buf_[i];    
    this->spi_dma_buffer_[buffer_index++] = bit_mask_table[0x0f & (led_color_byte >> 4)];
    this->spi_dma_buffer_[buffer_index++] = bit_mask_table[0x0f & (led_color_byte)];    
  }

  // Transmitting DMA buffer through SPI
  if (spi_device_transmit(this->spi_handle_, &this->spi_transaction_) != ESP_OK) {
    ESP_LOGE(TAG, "SPI TX error");
    this->status_set_warning();
    return;
  }    

  this->status_clear_warning();
}

void ESP32SPILEDStripLightOutput::dump_config() {  
  ESP_LOGCONFIG(TAG, "ESP32 SPI LED Strip:");
  ESP_LOGCONFIG(TAG, "  Pin: %u", this->pin_);  
  ESP_LOGCONFIG(TAG, "  Channel: %d", this->spi_dma_channel_);
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

light::ESPColorView ESP32SPILEDStripLightOutput::get_view_internal(int32_t index) const {
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

}  // namespace esp32_spi_led_strip
}  // namespace esphome

#endif  // USE_ESP32
