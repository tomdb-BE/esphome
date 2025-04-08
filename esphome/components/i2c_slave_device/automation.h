#pragma once

#include "esphome/core/automation.h"
#include "esphome/components/i2c_slave_device/i2c_slave_device.h"

#include <string>

namespace esphome {
namespace i2c_slave_device {

class I2CSlaveDeviceOnReceiveTrigger : public Trigger<std::string> {
 public:
  explicit I2CSlaveDeviceOnReceiveTrigger(I2CSlaveDevice *parent) {
    parent->add_on_rx_callback([this](const std::string &value) { this->check_value_(value); });
  }
  void set_trigger_always() { this->trigger_always_ = true; }
  void set_trigger_value(const std::string &trigger_value) { this->trigger_value_ = std::move(trigger_value); }

 protected:
  void check_value_(const std::string &new_value) {
    if (this->trigger_always_ || new_value == this->trigger_value_)
      this->trigger(std::move(new_value));
  }
  bool trigger_always_ = false;
  std::string trigger_value_;
};

template<typename... Ts> class I2CSlaveDeviceHasValueCondition : public Condition<Ts...> {
 public:
  I2CSlaveDeviceHasValueCondition(I2CSlaveDevice *parent) : parent_(parent) {}

  void set_value(std::string value) { this->value_ = std::move(value); }
  bool check(Ts... x) override { return this->value_ == this->parent_->read(); }

 protected:
  I2CSlaveDevice *parent_;
  std::string value_;
};

}  // namespace i2c_slave_device
}  // namespace esphome