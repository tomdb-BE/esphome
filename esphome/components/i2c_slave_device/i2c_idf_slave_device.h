#pragma once

#ifndef CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
#define CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
#endif

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

#include <driver/i2c.h>

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
  static const size_t CMD_BASE_SIZE = 2;

  ~I2CIDFSlaveDevice();

  void set_sda_pin(uint8_t pin_sda) { i2c_slave_config_.sda_io_num = gpio_num_t(pin_sda); }
  void set_scl_pin(uint8_t pin_scl) { i2c_slave_config_.scl_io_num = gpio_num_t(pin_scl); }
  void set_pullup_enabled_sda(bool pullup_enabled_sda) {
    i2c_slave_config_.sda_pullup_en = (pullup_enabled_sda) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
  }
  void set_pullup_enabled_scl(bool pullup_enabled_scl) {
    i2c_slave_config_.scl_pullup_en = (pullup_enabled_scl) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
  }
  void set_max_frequency(uint32_t max_frequency) { i2c_slave_config_.slave.maximum_speed = max_frequency; }
  void set_address(uint8_t address) { i2c_slave_config_.slave.slave_addr = (uint16_t) address; }
  void set_command_prefix(uint32_t command_prefix) { command_prefix_ = command_prefix; }
  void set_rx_buffer_size(uint8_t rx_buffer_size) { rx_buffer_size_ = (size_t) rx_buffer_size; }
  void set_tx_buffer_size(uint8_t tx_buffer_size) { tx_buffer_size_ = (size_t) tx_buffer_size; }
  void set_component(void *target = nullptr, TargetType type = TYPE_NONE, uint8_t target_id = 0,
                     std::string target_name = "<unknown>");

  float get_setup_priority() const override { return setup_priority::HARDWARE; }
  void setup() override;
  void dump_config() override;
  void update() override;

  int read_data();
  void get_data(uint8_t *data);
  std::string get_data();

  int set_data(uint8_t *data, size_t size = 0);
  int set_data(std::string data);

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
  i2c_config_t i2c_slave_config_{};
  i2c_port_t i2c_slave_port_{};
  std::map<uint8_t, I2CAction_ *> actions_{};
  bool ready_{false};
  size_t command_size_{CMD_BASE_SIZE};
  size_t data_received_size_{CMD_BASE_SIZE};
  size_t data_sent_size_{1};
  size_t prefix_size_{0};
  size_t rx_buffer_size_{1};
  size_t tx_buffer_size_{1};
  uint32_t command_prefix_{0};
  uint8_t *prefix_{nullptr};
  uint8_t *data_received_{nullptr};
  uint8_t *data_sent_{nullptr};
};

typedef I2CIDFSlaveDevice I2CSlaveDevice;

}  // namespace i2c_slave_device
}  // namespace esphome