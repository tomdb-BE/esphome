#pragma once

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"

#if defined(USE_ARDUINO)
#define I2C_NUM_MAX 2
#include <Wire.h>
#elif CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
#include <driver/i2c_slave.h>
#else
#include <driver/i2c.h>
#endif

#include <string>

namespace esphome {
namespace i2c_slave_device {

class I2CSlaveDevice : public Component {
 public:
  // Public interfaces

  // Reads <size> chars from the i2c rx buffer into a string. When size = 0, all chars are copied.
  std::string read(uint32_t size = 0);
  // Reads <size> bytes from the i2c rx buffer into a byte array. When size = 0, all bytes are copied.
  uint32_t read_raw(uint8_t *rx_byte_array, uint32_t size = 0);
  // Writes <size> bytes from a byte array in the I2C's tx buffer. When size = 0, all bytes are written.
  uint32_t write(const uint8_t *tx_byte_array, uint32_t size = 0);
  // Writes <size> chars from a string in the I2C's tx buffer. When size = 0, all chars are written.
  uint32_t write(const std::string tx_string, uint32_t size = 0);
  // Returns the time in millis since last succesful receive
  uint32_t get_last_receive_time() { return esphome::millis() - this->last_rx_time_; }
  // Returns the time in millis since last succesful request
  uint32_t get_last_request_time() { return esphome::millis() - this->last_tx_time_; }

  // Callbacks

#if defined(USE_ARDUINO)
  // (Dummy) static callback functions are required for Wire to operate in slave mode. Handling is done in the loop.
  // This avoids the need for a static definition of this class to allow multiple instances.
  static void i2c_slave_tx_callback(){};
  static void i2c_slave_rx_callback(int size){};
#endif

#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2  // ESP-IDF >= v5.4.x
  // I2C Slave v2 driver (ESP-IDF >= v5.4.x) allows providing a pointer to the instance in the callback function.
  // This allows for callbacks to multiple instances and avoids checking for new master requests in the loop.
  static bool i2c_slave_tx_callback(i2c_slave_dev_handle_t i2c_slave_handle,
                                    const i2c_slave_request_event_data_t *evt_data, void *arg);
  static bool i2c_slave_rx_callback(i2c_slave_dev_handle_t i2c_slave_handle,
                                    const i2c_slave_rx_done_event_data_t *evt_data, void *arg);
  // void handle_on_receive() { this->on_rx_callback_.call(); }
  void loop() override {
    if (this->new_rx_size_) {
      this->on_rx_callback_.call(this->read());
      this->new_rx_size_ = 0;
    }
  }
#else  // Arduino and ESP-IDF < v5.4.x
  // I2C Slave driver v1 on ESP-IDF and the Wire driver on Arduino don't allow per instance callbacks.
  // Therefore, checking for new receives/requests from master will be done in the loop.
  void loop() override { this->get_rx_buffer_(); }

#endif

  // Setup

  void set_sda_pin(uint8_t sda_pin) { this->sda_pin_ = static_cast<gpio_num_t>(sda_pin); }
  void set_scl_pin(uint8_t scl_pin) { this->scl_pin_ = static_cast<gpio_num_t>(scl_pin); }
  void set_pullup(bool pullup) { this->pullup_ = pullup; }
  void set_address(uint8_t address) { this->address_ = (uint16_t) address; }
  void set_rx_buffer_size(uint32_t rx_buffer_size) { this->rx_buffer_size_ = rx_buffer_size; }
  void set_tx_buffer_size(uint32_t tx_buffer_size) { this->tx_buffer_size_ = tx_buffer_size; }

  void add_on_rx_callback(std::function<void(const std::string &)> &&callback) {
    this->on_rx_callback_.add(std::move(callback));
  }

  float get_setup_priority() const override { return setup_priority::BUS; }
  void setup() override;
  void dump_config() override;
  ~I2CSlaveDevice();

 protected:
  CallbackManager<void(const std::string &)> on_rx_callback_;
  bool initialized_{false};
  gpio_num_t sda_pin_{GPIO_NUM_5};
  gpio_num_t scl_pin_{GPIO_NUM_6};
  uint16_t address_{0};
  bool pullup_{true};
  uint32_t rx_buffer_size_{256};
  uint32_t tx_buffer_size_{256};
  uint32_t new_tx_size_{0};
  uint32_t new_rx_size_{0};
  uint32_t last_tx_size_{0};
  uint32_t last_rx_size_{0};
  uint32_t last_tx_time_{0};
  uint32_t last_rx_time_{0};
  uint8_t *rx_buffer_{nullptr};
  uint8_t *tx_buffer_{nullptr};
#if defined(USE_ARDUINO)
  TwoWire *wire_{nullptr};
#else
  i2c_port_t i2c_slave_port_{I2C_NUM_0};
#endif
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  i2c_slave_dev_handle_t i2c_slave_dev_handle_{nullptr};
#else
  void get_rx_buffer_();
  void set_tx_buffer_();
#endif
};

}  // namespace i2c_slave_device
}  // namespace esphome