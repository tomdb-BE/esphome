#pragma once

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

class I2CSlaveDevice : public PollingComponent {
 public:
  ~I2CSlaveDevice();
  void set_sda_pin(uint8_t sda_pin) { this->sda_pin_ = static_cast<gpio_num_t>(sda_pin); }
  void set_scl_pin(uint8_t scl_pin) { this->scl_pin_ = static_cast<gpio_num_t>(scl_pin); }
  void set_pullup(bool pullup) { this->pullup_ = pullup; }
  void set_address(uint8_t address) { this->address_ = (uint16_t) address; }
  void set_rx_buffer_size(uint16_t rx_buffer_size) { this->rx_buffer_size_ = (size_t) rx_buffer_size; }
  void set_tx_buffer_size(uint16_t tx_buffer_size) { this->tx_buffer_size_ = (size_t) tx_buffer_size; }

  float get_setup_priority() const override { return setup_priority::DATA; }
  void setup() override;
  void setup_man();
  void dump_config() override;
  void update() override;

  int read_data(size_t size = 0);
  void get_data(uint8_t *data, size_t size = 0);
  std::string get_data(size_t size = 0);

  int write_data(size_t size);
  int write_data(std::string data);

#if defined(USE_ARDUINO)
  static void i2c_slave_tx_callback(void *arg);
  static void i2c_slave_rx_callback(int size, void *arg);
  // A very hacky way to override the private pointers to the Wire callback functions.
  // This allows for multiple I2CSlaveDevice instances as we can pass a pointer of the instance
  // to the callback function. (similar to esp-idf v2 driver method)
  class TwoWireExtended : public TwoWire {
   public:
    TwoWireExtended(uint8_t bus_num) : TwoWire(bus_num) {
      this->onRequest(this->_onRequestCallBackDummy);
      this->onReceive(this->_onReceiveCallBackDummy);
    }

    void onRequestExt(void (*function)(I2CSlaveDevice *)) { this->_onRequestCallbackExt = function; }
    void onReceiveExt(void (*function)(int, I2CSlaveDevice *)) { this->_onReceiveCallbackExt = function; }

   protected:
    I2CSlaveDevice *i2c_slave_dev_{nullptr};
    static void _onRequestCallBackDummy() { _onRequestCallbackExt(); }
    static void _onReceiveCallBackDummy(int size) { _onReceiveCallbackExt(); }
    static void (*_onRequestCallbackExt)(void *);
    static void (*_onReceiveCallbackExt)(int, void *);
  };
#elif CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  static bool i2c_slave_tx_callback(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_request_event_data_t *evt_data,
                                    void *arg);
  static bool i2c_slave_rx_callback(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_rx_done_event_data_t *evt_data,
                                    void *arg);
#endif

 protected:
  bool initialized_{false};
  gpio_num_t sda_pin_{GPIO_NUM_5};
  gpio_num_t scl_pin_{GPIO_NUM_6};
  uint16_t address_{0};
  bool pullup_{false};
  size_t rx_buffer_size_{101};
  size_t tx_buffer_size_{101};
  uint8_t *rx_buffer_{nullptr};
  uint8_t *tx_buffer_{nullptr};
#if defined(USE_ARDUINO)
  TwoWireExtended *wire_{nullptr};
#else
  i2c_port_t i2c_slave_port_{I2C_NUM_0};
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  i2c_slave_dev_handle_t i2c_slave_dev_handle_{nullptr};
#endif  // CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
#endif  // USE_ARDUINO
};

}  // namespace i2c_slave_device
}  // namespace esphome