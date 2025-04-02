#pragma once

#include "esphome/core/component.h"
#include "esphome/core/string_ref.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/cover/cover.h"
#include "esphome/components/light/light_state.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/template/binary_sensor/template_binary_sensor.h"
#include "esp_idf_version.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(8, 4, 1)  // ESP_IDF_VERSION >= 5.4.1
#ifndef CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2     // driver v2 not defined
#define CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2 1   // define and enable I2C Slave v2 driver
#endif                                               // driver v2 not defined
#else
#define CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2 0
#endif  // ESP_IDF_VERSION

#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
#include <driver/i2c_slave.h>
#else
#include <driver/i2c.h>
#endif

#include <string>
#include <map>

namespace esphome {
namespace i2c_slave_device {

enum TargetType {
  TYPE_NONE,
  BINARY_SENSOR,
  BINARY_SENSOR_TEMPLATE,
  BUTTON,
  COVER,
  LIGHT,
  SENSOR,
  SWITCH,
  TEXT_SENSOR,
  INVALID_TYPE,
};

enum TargetAction {
  ACTION_NONE,
  TURN_OFF,
  TURN_ON,
  TOGGLE,
  CLOSE,
  OPEN,
  STOP,
  PRESS,
  GET,
  SET,
  INCREASE,
  DECREASE,
  INVALID_ACTION,
};

const char *const str_target_actions[] = {
    "NONE",  "TURN_OFF", "TURN_ON", "TOGGLE",   "CLOSE",    "OPEN",    "STOP",
    "PRESS", "GET",      "SET",     "INCREASE", "DECREASE", "INVALID",
};

enum TargetProperty {
  PROPERTY_NONE,
  STATE,
  BRIGHTNESS,
  RED,
  GREEN,
  BLUE,
  WHITE,
  COLD_WHITE,
  WARM_WHITE,
  COLOR_TEMPERATURE,
  EFFECT,
  POSITION,
  TILT,
  INVALID_PROPERTY,
};

const char *const str_target_properties[] = {
    "NONE",       "STATE",      "BRIGHTNESS",        "RED",    "GREEN",    "BLUE", "WHITE",
    "COLD_WHITE", "WARM_WHITE", "COLOR_TEMPERATURE", "EFFECT", "POSITION", "TILT", "INVALID",
};

class I2CIDFSlaveDevice : public PollingComponent {
 public:
  ~I2CIDFSlaveDevice();

  void set_sda_pin(uint8_t sda_pin) { this->sda_pin_ = (gpio_num_t) sda_pin; }
  void set_scl_pin(uint8_t scl_pin) { this->scl_pin_ = (gpio_num_t) scl_pin; }
  void set_pullup(bool pullup) { this->pullup_ = pullup; }
  void set_address(uint8_t address) { this->address_ = (uint16_t) address; }
  void set_rx_buffer_size(uint16_t rx_buffer_size) { this->rx_buffer_size_ = (size_t) rx_buffer_size; }
  void set_tx_buffer_size(uint16_t tx_buffer_size) { this->tx_buffer_size_ = (size_t) tx_buffer_size; }
  void set_prefix(uint32_t prefix);
  void set_component(void *target = nullptr, TargetType type = TYPE_NONE, uint8_t target_id = 0,
                     std::string target_name = "<unknown>");

  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  void setup() override;
  void dump_config() override;
  void update() override;

  int read_data(size_t size = 0);
  void get_data(uint8_t *data, size_t size = 0);
  std::string get_data(size_t size = 0);

  int write_data(size_t size);
  int write_data(std::string data);

#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  static bool i2c_slave_rx_callback(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_rx_done_event_data_t *rx_data,
                                    void *arg);
  void handle_rx_event(const i2c_slave_rx_done_event_data_t *rx_buffer);
#endif

 private:
  class I2CAction_ {
   public:
    virtual size_t get(TargetProperty property, uint8_t *data, const size_t max_size = 1) { return 0; }
    virtual void set(TargetProperty property, int16_t value, bool relative) {}
    virtual void trigger(TargetAction action) {}
    std::string target_name{"<unknown>"};
  };

  class I2CActionBinarySensor_ : public I2CAction_ {
   public:
    I2CActionBinarySensor_(esphome::binary_sensor::BinarySensor *target) { target_ = target; }
    size_t get(TargetProperty property, uint8_t *data, const size_t max_size = 1) override {
      data[0] = target_->state;
      return 1;
    }

   private:
    esphome::binary_sensor::BinarySensor *target_{nullptr};
  };

  class I2CActionBinarySensorTemplate_ : public I2CAction_ {
   public:
    I2CActionBinarySensorTemplate_(esphome::template_::TemplateBinarySensor *target) { target_ = target; }
    size_t get(TargetProperty property, uint8_t *data, const size_t max_size = 1) override {
      data[0] = target_->state;
      return 1;
    }

   private:
    esphome::template_::TemplateBinarySensor *target_{nullptr};
  };

  class I2CActionButton_ : public I2CAction_ {
   public:
    I2CActionButton_(esphome::button::Button *target) { target_ = target; }
    void trigger(TargetAction action) override { target_->press(); }

   private:
    esphome::button::Button *target_{nullptr};
  };

  class I2CActionCover_ : public I2CAction_ {
   public:
    I2CActionCover_(esphome::cover::Cover *target) { target_ = target; }
    size_t get(TargetProperty property, uint8_t *data, const size_t max_size = 1) override;
    void set(TargetProperty property, int16_t value, bool relative) override;
    void trigger(TargetAction action) override;

   private:
    esphome::cover::Cover *target_{nullptr};
  };

  class I2CActionLight_ : public I2CAction_ {
   public:
    I2CActionLight_(esphome::light::LightState *target) { target_ = target; }
    size_t get(TargetProperty property, uint8_t *data, const size_t max_size = 1) override;
    void set(TargetProperty property, int16_t value, bool relative) override;
    void trigger(TargetAction action) override;

   private:
    esphome::light::LightState *target_{nullptr};
  };

  class I2CActionSensor_ : public I2CAction_ {
   public:
    I2CActionSensor_(esphome::sensor::Sensor *target) { target_ = target; }
    size_t get(TargetProperty property, uint8_t *data, const size_t max_size = 1) override;

   private:
    esphome::sensor::Sensor *target_{nullptr};
  };

  class I2CActionSwitch_ : public I2CAction_ {
   public:
    I2CActionSwitch_(esphome::switch_::Switch *target) { target_ = target; }
    size_t get(TargetProperty property, uint8_t *data, const size_t max_size = 1) override {
      data[0] = target_->state;
      return 1;
    }
    void trigger(TargetAction action) override;

   private:
    esphome::switch_::Switch *target_{nullptr};
  };
  class I2CActionTextSensor_ : public I2CAction_ {
   public:
    I2CActionTextSensor_(esphome::text_sensor::TextSensor *target) { target_ = target; }
    size_t get(TargetProperty property, uint8_t *data, const size_t max_size = 1) override;
    void set(TargetProperty property, int16_t value, bool relative) override;

   private:
    esphome::text_sensor::TextSensor *target_{nullptr};
  };

 protected:
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  i2c_slave_dev_handle_t i2c_slave_dev_handle_{nullptr};
#endif
  i2c_port_t i2c_slave_port_{I2C_NUM_0};
  gpio_num_t sda_pin_{GPIO_NUM_5};
  gpio_num_t scl_pin_{GPIO_NUM_6};
  std::map<uint8_t, I2CAction_ *> actions_{};
  bool ready_{false};
  bool pullup_{false};
  size_t command_size_{0};
  size_t data_received_size_{0};
  size_t data_sent_size_{1};
  size_t prefix_size_{0};
  size_t rx_buffer_size_{1};
  size_t tx_buffer_size_{1};
  uint16_t address_{0};
  uint8_t prefix_[4]{0};
  uint8_t *data_received_{nullptr};
  uint8_t *data_sent_{nullptr};
};

typedef I2CIDFSlaveDevice I2CSlaveDevice;

}  // namespace i2c_slave_device
}  // namespace esphome