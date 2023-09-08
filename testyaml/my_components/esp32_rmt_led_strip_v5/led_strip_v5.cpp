#ifdef USE_ESP32

#include <cinttypes>
#include "led_strip_v5.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <esp_attr.h>

namespace esphome {
namespace esp32_rmt_led_strip_v5 {

static const char *const TAG = "esp32_rmt_led_strip";

size_t ESP32RMTLEDStripLightOutputv5::rmt_encode_led_strip_(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t encode_state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    switch (led_encoder->state) {
    case 0: // send RGB data
        encoded_symbols += led_encoder->bytes_encoder->encode(led_encoder->bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = RMT_ENCODING_COMPLETE; // switch to next state when current encoding session finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            encode_state = static_cast<rmt_encode_state_t>(encode_state | RMT_ENCODING_MEM_FULL);
            goto out; // yield if there's no free space for encoding artifacts
        }
    // fall-through
    case 1: // send reset code
        encoded_symbols += led_encoder->copy_encoder->encode(led_encoder->copy_encoder, channel, &led_encoder->reset_code, sizeof(led_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = RMT_ENCODING_RESET; // back to the initial encoding session
            encode_state = static_cast<rmt_encode_state_t>(encode_state | RMT_ENCODING_COMPLETE);            
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {            
            encode_state = static_cast<rmt_encode_state_t>(encode_state | RMT_ENCODING_MEM_FULL);
            goto out; // yield if there's no free space for encoding artifacts
        }
    }
out:
    *ret_state = encode_state;
    return encoded_symbols;
}

esp_err_t ESP32RMTLEDStripLightOutputv5::rmt_del_led_strip_encoder_(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

esp_err_t ESP32RMTLEDStripLightOutputv5::rmt_led_strip_encoder_reset_(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

esp_err_t ESP32RMTLEDStripLightOutputv5::rmt_setup_led_strip_encoder_(rmt_encoder_handle_t *ret_encoder_handle) {  
  
   this->led_strip_encoder_.reset_code = {
    .duration0 = this->reset_ticks_,
    .level0 = 0,
    .duration1 = this->reset_ticks_,      
    .level1 = 0,
  };
  this->led_strip_encoder_.base.encode = &this->rmt_encode_led_strip_;
  this->led_strip_encoder_.base.del = &this->rmt_del_led_strip_encoder_;
  this->led_strip_encoder_.base.reset = &this->rmt_led_strip_encoder_reset_;
     
  if (rmt_new_bytes_encoder(&this->bytes_encoder_config_, &this->led_strip_encoder_.bytes_encoder) != ESP_OK) {
    ESP_LOGE(TAG, "Cannot initialize RMT Byte Encoder!");
    this->mark_failed();
    return ESP_FAIL;
  }

  if (rmt_new_copy_encoder(&this->copy_encoder_config, &this->led_strip_encoder_.copy_encoder) != ESP_OK) {
    ESP_LOGE(TAG, "Cannot initialize RMT Copy Encoder!");
    this->mark_failed();
    return ESP_FAIL;
  }
  
  *ret_encoder_handle = &this->led_strip_encoder_.base;
  return ESP_OK;
}

void ESP32RMTLEDStripLightOutputv5::setup() {
  ESP_LOGCONFIG(TAG, "Setting up ESP32 LED Strip - ESP-IDF > 5.x...");

  size_t buffer_size = this->get_buffer_size_();

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

  this->channel_config_ = {
    .gpio_num = gpio_num_t(this->pin_),
    .clk_src = RMT_CLK_SRC_APB,
    .resolution_hz = APB_CLK_FREQ / RMT_CLK_DIV,
    .mem_block_symbols = 256,    
    .trans_queue_depth = 2,
    .flags = {
      .invert_out = 0,   /*!< Whether to invert the RMT channel signal before output to GPIO pad */
      .with_dma = 0,     /*!< If set, the driver will allocate an RMT channel with DMA capability */
      .io_loop_back = 0, /*!< The signal output from the GPIO will be fed to the input path as well */
      .io_od_mode = 0,    /*!< Configure the GPIO as open-drain mode */
    },    
  };

  this->transmit_config_ = {
    .loop_count = 0,
    .flags = {
      .eot_level = 0,   /*!< Whether to invert the RMT channel signal before output to GPIO pad */      
    },
  };
  
  if (rmt_new_tx_channel(&this->channel_config_, &this->channel_) != ESP_OK) {
    ESP_LOGE(TAG, "Cannot initialize RMT channel!");
    this->mark_failed();
    return;
  }

  if (rmt_enable(this->channel_) != ESP_OK) {
    ESP_LOGE(TAG, "Cannot enable RMT channel!");
    this->mark_failed();
    return;
  }

  if (rmt_setup_led_strip_encoder_(&this->led_strip_encoder_handle_) != ESP_OK) {
   ESP_LOGE(TAG, "Cannot initialize RMT Encoder!");
    this->mark_failed();
    return;
  }  
}

void ESP32RMTLEDStripLightOutputv5::set_led_params(uint32_t bit0_high, uint32_t bit0_low, uint32_t bit1_high,
                                                 uint32_t bit1_low) {
                                           
  float ratio = (float) APB_CLK_FREQ / RMT_CLK_DIV / 1e09f;
  
  this->bytes_encoder_config_ = {
    .bit0 = {
      .duration0 = (uint16_t) (ratio * bit0_high),
      .level0 = 1,
      .duration1 = (uint16_t) (ratio * bit0_low),
      .level1 = 0,
      },
    .bit1 = {
      .duration0 = (uint16_t) (ratio * bit1_high),
      .level0 = 1,
      .duration1 = (uint16_t) (ratio * bit1_low),
      .level1 = 0,
    },
    .flags = {
      .msb_first = 1,
    },
  };

}
void ESP32RMTLEDStripLightOutputv5::write_state(light::LightState *state) {
  // protect from refreshing too often
  uint32_t now = micros();
  if (*this->max_refresh_rate_ != 0 && (now - this->last_refresh_) < *this->max_refresh_rate_) {
    // try again next loop iteration, so that this change won't get lost
    this->schedule_show();
    return;
  }

  if (rmt_tx_wait_all_done(this->channel_, 100) != ESP_OK) {
    ESP_LOGE(TAG, "RMT TX timeout");
    this->status_set_warning();
    return;
  }

  this->last_refresh_ = now;
  this->mark_shown_();

  ESP_LOGVV(TAG, "Writing RGB values to bus...");  

  size_t buffer_size = this->get_buffer_size_();

  if (rmt_transmit(this->channel_, this->led_strip_encoder_handle_, this->buf_, buffer_size, &this->transmit_config_) != ESP_OK) {
    ESP_LOGE(TAG, "RMT TX error");
    this->status_set_warning();
    return;
  }

  this->status_clear_warning();
}

light::ESPColorView ESP32RMTLEDStripLightOutputv5::get_view_internal(int32_t index) const {
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

void ESP32RMTLEDStripLightOutputv5::dump_config() {  
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

float ESP32RMTLEDStripLightOutputv5::get_setup_priority() const { return setup_priority::HARDWARE; }

}  // namespace esp32_rmt_led_strip_v5
}  // namespace esphome

#endif  // USE_ESP32
