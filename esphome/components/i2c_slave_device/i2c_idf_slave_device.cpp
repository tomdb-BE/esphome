#include "i2c_idf_slave_device.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace i2c_slave_device {

static const char *const TAG = "i2c_slave_device";

// HELPERS

// Converts unsigned 32-bit int into smallest byte array, allocates memory and return a pointer to the byte_array
static uint8_t uint32_to_byte_array_(uint8_t *&byte_array, uint32_t int_value) {
  uint8_t size = 0;

  if (byte_array)
    return 0;

  // if the unsigned int is 0, allocate an array of size 1 and set to 0
  if (!int_value) {
    byte_array = new uint8_t[1];
    byte_array[0] = 0;
    return 1;
  }

  for (int i = sizeof(uint32_t) - 1; i >= 0; i--) {
    uint8_t byte_value = (int_value >> i * 8) & 0xFF;
    // if octet > 0, alloc byte_array if not allocated with size = first non-zero octet
    if (!byte_array && byte_value) {
      size = i + 1;
      byte_array = new uint8_t[size];
    }
    if (byte_array)
      byte_array[size - 1 - i] = byte_value;
  }

  return size;
}

// Create a new buffer with specified size and set contents to zero. Delete if already exists.
static void initialize_byte_buffer(uint8_t *&byte_array, size_t size) {
  if (byte_array)
    delete[] byte_array;
  byte_array = new uint8_t[size];
  for (int i = 0; i < size; i++)
    byte_array[i] = 0;
  return;
}

// Percentage (float) to byte
static uint8_t percent_float_to_byte(float value) { return (uint8_t) (value * 255.0f); }

// Convert byte-range to float percentage.
static float state_byte_to_float(int16_t value) { return ((float) value) / 255.0f; }

// Return sum of two floats, 0.0f if negative, 1.0f if greater than 1.0f
static void get_diff_value(float *old_value, float *new_value) {
  *new_value += *old_value;
  if (*new_value > 1.0f)
    *new_value = 1.0f;
  if (*new_value < 0.0f)
    *new_value = 0.0f;
}

// Converts a byte_array to a string of hex values
static void get_hex_string(char *&hex_string, const uint8_t *byte_array, size_t byte_array_size) {
  for (size_t i = 0; i < byte_array_size; i++) {
    sprintf(hex_string + i * 5, "0x%02x ", byte_array[i]);
  }
}

// CALL BACKS

void IRAM_ATTR i2c_slave_isr_handler_rx(void *arg) {
  ESP_LOGI(TAG, "I2C callback triggered!");
  /*
  i2c_intr_event_t evt_type = I2C_INTR_EVENT_ERR;
  i2c_ll_slave_get_event(&I2C0, &evt_type);
  if (evt_type == I2C_INTR_EVENT_TRANS_DONE || evt_type == I2C_INTR_EVENT_RXFIFO_FULL) {
    ESP_LOGD(TAG, "Data received from master");
  }
  */
}

// SETUP

void I2CIDFSlaveDevice::setup() {
  ESP_LOGI(TAG, "Setting up I2C Slave Bus...");
  esp_err_t err;

  // Set slave mode in I2C config
  this->i2c_slave_config_.mode = I2C_MODE_SLAVE;
  // Disable 10-bit mode
  this->i2c_slave_config_.slave.addr_10bit_en = 0;

  // Find and set free port
  static i2c_port_t next_port = I2C_NUM_0;
  this->i2c_slave_port_ = next_port;
#if SOC_I2C_NUM > 1
  next_port = (next_port == I2C_NUM_0) ? I2C_NUM_1 : I2C_NUM_MAX;
#else
  next_port = I2C_NUM_MAX;
#endif
  if (this->i2c_slave_port_ == I2C_NUM_MAX) {
    ESP_LOGE(TAG, "Too many I2C buses configured. Max %u supported.", SOC_I2C_NUM);
    mark_failed();
    return;
  }

  // Initialize I2C in slave mode
  err = i2c_param_config(this->i2c_slave_port_, &this->i2c_slave_config_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register on port %d. Error: %d", i2c_slave_port_, err);
    mark_failed();
    return;
  }

  // Set the prefix buffer
  this->prefix_size_ = uint32_to_byte_array_(this->prefix_, this->command_prefix_);

  // Calculate the total command size
  this->command_size_ = this->prefix_size_ + CMD_BASE_SIZE;
  this->data_received_size_ = this->command_size_ * this->rx_buffer_size_;
  this->data_sent_size_ = this->tx_buffer_size_;

  // tx and rx buffer size needs to be > 100 for I2C esp-idf driver in slave mode
  size_t internal_rx_buffer_size = (this->data_received_size_ < 101) ? 101 : this->data_received_size_;
  size_t internal_tx_buffer_size = (this->data_sent_size_ < 101) ? 101 : this->data_sent_size_;

  // Install the I2C driver
  err = i2c_driver_install(this->i2c_slave_port_, this->i2c_slave_config_.mode, internal_rx_buffer_size,
                           internal_tx_buffer_size, 0);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to install I2C slave driver. Error: %d", err);
    delete[] prefix_;
    mark_failed();
    return;
  }

  // Allocate buffers
  initialize_byte_buffer(data_received_, data_received_size_);
  initialize_byte_buffer(data_sent_, data_sent_size_);

  ESP_LOGI(TAG, "I2C Slave ready to receive on address %d", i2c_slave_config_.slave.slave_addr);
  ready_ = true;
}

// Setup actions for each component based on type
void I2CIDFSlaveDevice::set_component(void *target, TargetType type, uint8_t target_id, std::string target_name) {
  if (!target_id)
    return;

  switch (type) {
    case BINARY_SENSOR: {
      // Reading state from template binary sensor object requires cast to template type. Type is checked first.
      esphome::template_::TemplateBinarySensor *test_binary_sensor =
          (esphome::template_::TemplateBinarySensor *) target;
      if (test_binary_sensor->get_component_source() == "template.binary_sensor") {
        I2CActionBinarySensorTemplate_ *i2c_action =
            new I2CActionBinarySensorTemplate_((esphome::template_::TemplateBinarySensor *) target);
        i2c_action->target_name = target_name;
        actions_.emplace(target_id, i2c_action);
      } else {
        I2CActionBinarySensor_ *i2c_action =
            new I2CActionBinarySensor_((esphome::binary_sensor::BinarySensor *) target);
        i2c_action->target_name = target_name;
        actions_.emplace(target_id, i2c_action);
      }
      break;
    }
    case BUTTON: {
      I2CActionButton_ *i2c_action = new I2CActionButton_((esphome::button::Button *) target);
      i2c_action->target_name = target_name;
      actions_.emplace(target_id, i2c_action);
      break;
    }
    case COVER: {
      I2CActionCover_ *i2c_action = new I2CActionCover_((esphome::cover::Cover *) target);
      actions_.emplace(target_id, i2c_action);
      i2c_action->target_name = target_name;
      break;
    }
    case LIGHT: {
      I2CActionLight_ *i2c_action = new I2CActionLight_((esphome::light::LightState *) target);
      i2c_action->target_name = target_name;
      actions_.emplace(target_id, i2c_action);
      break;
    }
    case SENSOR: {
      I2CActionSensor_ *i2c_action = new I2CActionSensor_((esphome::sensor::Sensor *) target);
      i2c_action->target_name = target_name;
      actions_.emplace(target_id, i2c_action);
      break;
    }
    case SWITCH: {
      I2CActionSwitch_ *i2c_action = new I2CActionSwitch_((esphome::switch_::Switch *) target);
      i2c_action->target_name = target_name;
      actions_.emplace(target_id, i2c_action);
      break;
    }
    case TEXT_SENSOR: {
      I2CActionTextSensor_ *i2c_action = new I2CActionTextSensor_((esphome::text_sensor::TextSensor *) target);
      i2c_action->target_name = target_name;
      actions_.emplace(target_id, i2c_action);
      break;
    }
    default:
      return;
  }
}

// UPDATE - COMMAND PROCESSING

void I2CIDFSlaveDevice::update() {
  // Read the rx buffers of the i2c device for new commands from master
  int rx_data_size = read_data();

  // Stop processing if data read size is smaller than minimum command size
  if (rx_data_size < command_size_)
    return;

  // Loop through the received data
  for (int i = 0; i <= rx_data_size - command_size_; i++) {
    // Check if the prefix is valid
    for (int j = 0; j < prefix_size_; j++) {
      if (prefix_[j] != data_received_[j + i]) {
        ESP_LOGD(TAG, "Invalid prefix byte: 0x%02x, expected: 0x%02x", data_received_[j + i], prefix_[i]);
        continue;
      }
      i++;
    }

    // Check if the target component is configured
    uint8_t target = data_received_[i];
    if (!actions_[target]) {
      ESP_LOGD(TAG, "Invalid target ID: 0x%02x", target);
      continue;
    }

    // Check if the requested action type is valid
    if (!data_received_[i + 1] || data_received_[i + 1] >= INVALID_ACTION) {
      ESP_LOGD(TAG, "Invalid action: 0x%02x", data_received_[i + 1]);
      continue;
    }
    i++;
    TargetAction action = (TargetAction) data_received_[i];

    // Trigger the action if no property and/or value is required. (not a GET/SET action)
    if (action < GET) {
      actions_[target]->trigger(action);
      ESP_LOGD(TAG, "Target ID 0x%02x: triggered action %s", target, str_target_actions[action]);
      continue;
    }

    // Check if the property to SET/GET is valid, if not, default to SET/GET the state
    TargetProperty property = STATE;
    if (i + 1 < rx_data_size && data_received_[i + 1] && data_received_[i + 1] < INVALID_PROPERTY) {
      i++;
      property = (TargetProperty) data_received_[i];
    }

    // Get the requested property from the component and write it to the tx buffer so the master can read it
    if (action == GET) {
      size_t data_sent_size = actions_[target]->get(property, data_sent_, data_sent_size_);
      set_data(data_sent_, data_sent_size);
      ESP_LOGD(TAG, "Target ID 0x%02x: wrote property %s", target, str_target_properties[property]);
      continue;
    }

    // Stop processing when we have reached the end of the read buffer and no value is available for the action
    if (i + 1 >= rx_data_size) {
      ESP_LOGD(TAG, "Target ID 0x%02x: cannot set property %s, no value provided", target,
               str_target_properties[property]);
      break;
    }

    // Handle the SET/INCREASE/DECREASE actions
    i++;
    // Cast to a signed 16 bit int to allow for negative values when action is DECREASE
    int16_t value = (int16_t) data_received_[i];
    // Set the relative flag if the action is INCREASE or DECREASE
    bool relative = (action == INCREASE || action == DECREASE);
    // Set the relative value to a negative number when the action is DECREASE
    if (action == DECREASE)
      value = 0 - value;
    // Trigger the action on the defined property with the defined value
    actions_[target]->set(property, value, relative);
    ESP_LOGD(TAG, "Target ID 0x%02x: SET %s with %s value %d", target, str_target_properties[property],
             (relative) ? "relative" : "absolute", value);
  }
}

// I2C OPERATIONS

// Reads data sent by the master from the i2c rx buffer into the data_received_ buffer and return the size
int I2CIDFSlaveDevice::read_data() {
  if (!ready_)
    return -2;
  return i2c_slave_read_buffer(i2c_slave_port_, data_received_, data_received_size_, 0);
}

// Reads data sent by the master from the i2c rx buffer into a byte-array
void I2CIDFSlaveDevice::get_data(uint8_t *data) {
  int rx_data_size = read_data();
  if (rx_data_size <= 0)
    return;

  for (int i = 0; i < rx_data_size; i++)
    data[i] = data_received_[i];
}

// Reads data sent by the master from the i2c rx buffer into a string
std::string I2CIDFSlaveDevice::get_data() {
  int data_rx_size = read_data();

  if (data_rx_size == -2)
    return "NOT READY";

  if (data_rx_size < 0)
    return "ERROR";

  std::string received_data = "";
  for (int i = 0; i < data_rx_size; i++)
    received_data += data_received_[i];

  return received_data;
}

// Writes byte-array to the i2c tx buffer to be read by master
int I2CIDFSlaveDevice::set_data(uint8_t *data, size_t size) {
  if (!ready_)
    return -2;
  if (!data)
    return -1;
  if (!size)
    size = data_sent_size_;
  return i2c_slave_write_buffer(i2c_slave_port_, data, size, 0);
}

// Writes string to the i2c tx buffer to be read by master
int I2CIDFSlaveDevice::set_data(std::string data) {
  size_t size = (data.length() < data_sent_size_) ? data.length() : data_sent_size_;
  for (int i = 0; i < size; i++)
    data_sent_[i] = data[i];
  return set_data(data_sent_, size);
}

// ACTION HANDLERS

// COVER actions

size_t I2CIDFSlaveDevice::I2CActionCover_::get(TargetProperty property, uint8_t *data, const size_t max_size) {
  switch (property) {
    case STATE:
      data[0] = (target_->position > 0.0f) ? 1 : 0;
      break;
    case POSITION:
      data[0] = percent_float_to_byte(target_->position);
      break;
    case TILT:
      data[0] = percent_float_to_byte(target_->tilt);
      break;
    default:
      return 0;
  }
  return 1;
}

void I2CIDFSlaveDevice::I2CActionCover_::set(TargetProperty property, int16_t value, bool relative) {
  float new_value = state_byte_to_float(value);
  switch (property) {
    case POSITION:
      if (relative) {
        float old_value = percent_float_to_byte(target_->position);
        get_diff_value(&old_value, &new_value);
      }
      target_->make_call().set_position(new_value).perform();
      break;
    case TILT:
      if (relative) {
        float old_value = percent_float_to_byte(target_->tilt);
        get_diff_value(&old_value, &new_value);
      }
      target_->make_call().set_tilt(new_value).perform();
      break;
    default:
      return;
  }
}

void I2CIDFSlaveDevice::I2CActionCover_::trigger(TargetAction action) {
  switch (action) {
    case OPEN:
      target_->make_call().set_command_open().perform();
      break;
    case CLOSE:
      target_->make_call().set_command_close().perform();
      break;
    case TOGGLE:
      target_->make_call().set_command_toggle().perform();
      break;
    case STOP:
      target_->make_call().set_command_stop().perform();
      break;
    default:
      return;
  }
}

// LIGHT actions

size_t I2CIDFSlaveDevice::I2CActionLight_::get(TargetProperty property, uint8_t *data, const size_t max_size) {
  switch (property) {
    case BRIGHTNESS:
      data[0] = percent_float_to_byte(target_->current_values.get_brightness());
      break;
    case RED:
      data[0] = percent_float_to_byte(target_->current_values.get_red());
      break;
    case GREEN:
      data[0] = percent_float_to_byte(target_->current_values.get_green());
      break;
    case BLUE:
      data[0] = percent_float_to_byte(target_->current_values.get_blue());
      break;
    case WHITE:
      data[0] = percent_float_to_byte(target_->current_values.get_white());
      break;
    case COLD_WHITE:
      data[0] = percent_float_to_byte(target_->current_values.get_cold_white());
      break;
    case WARM_WHITE:
      data[0] = percent_float_to_byte(target_->current_values.get_warm_white());
      break;
    case COLOR_TEMPERATURE:
      data[0] = percent_float_to_byte(target_->current_values.get_color_temperature());
      break;
    case EFFECT: {
      if (!target_->supports_effects())
        return 0;
      std::string effect = target_->get_effect_name();
      size_t size = (effect.length() < max_size) ? effect.length() : max_size;
      for (int i = 0; i < size; i++)
        data[i] = effect[i];
      return size;
    }
    default:
      return 0;
  }
  return 1;
}

void I2CIDFSlaveDevice::I2CActionLight_::set(TargetProperty property, int16_t value, bool relative) {
  float new_value = state_byte_to_float(value);
  switch (property) {
    case BRIGHTNESS:
      if (relative) {
        float old_value = target_->current_values.get_brightness();
        get_diff_value(&old_value, &new_value);
      }
      target_->make_call().set_brightness_if_supported(new_value).perform();
      break;
    case RED:
      if (relative) {
        float old_value = target_->current_values.get_red();
        get_diff_value(&old_value, &new_value);
      }
      target_->make_call().set_red_if_supported(new_value).perform();
      break;
    case GREEN:
      if (relative) {
        float old_value = target_->current_values.get_green();
        get_diff_value(&old_value, &new_value);
      }
      target_->make_call().set_green_if_supported(new_value).perform();
      break;
    case BLUE:
      if (relative) {
        float old_value = target_->current_values.get_blue();
        get_diff_value(&old_value, &new_value);
      }
      target_->make_call().set_blue_if_supported(new_value).perform();
      break;
    case WHITE:
      if (relative) {
        float old_value = target_->current_values.get_white();
        get_diff_value(&old_value, &new_value);
      }
      target_->make_call().set_white_if_supported(new_value).perform();
      break;
    case COLD_WHITE:
      if (relative) {
        float old_value = target_->current_values.get_cold_white();
        get_diff_value(&old_value, &new_value);
      }
      target_->make_call().set_cold_white_if_supported(new_value).perform();
      break;
    case WARM_WHITE:
      if (relative) {
        float old_value = target_->current_values.get_warm_white();
        get_diff_value(&old_value, &new_value);
      }
      target_->make_call().set_warm_white_if_supported(new_value).perform();
      break;
    case COLOR_TEMPERATURE:
      if (relative) {
        float old_value = target_->current_values.get_color_temperature();
        get_diff_value(&old_value, &new_value);
      }
      target_->make_call().set_color_temperature_if_supported(new_value).perform();
      break;
    case EFFECT:
      if (!target_->supports_effects())
        return;
      target_->make_call().set_effect((uint32_t) value).perform();
      break;
    default:
      return;
  }
}

void I2CIDFSlaveDevice::I2CActionLight_::trigger(TargetAction action) {
  switch (action) {
    case TURN_ON:
      target_->turn_on().perform();
      ;
      break;
    case TURN_OFF:
      target_->turn_off().perform();
      ;
      break;
    case TOGGLE:
      target_->toggle().perform();
      break;
    default:
      return;
  }
}

// SENSOR actions

size_t I2CIDFSlaveDevice::I2CActionSensor_::get(TargetProperty property, uint8_t *data, const size_t max_size) {
  size_t size = sizeof(float);
  uint8_t *state = (uint8_t *) &target_->state;
  for (int i = size - 1; i >= 0; i--) {
    data[i] = state[i];
  }
  return size;
}

void I2CIDFSlaveDevice::I2CActionSwitch_::trigger(TargetAction action) {
  switch (action) {
    case TURN_ON:
      target_->turn_on();
      break;
    case TURN_OFF:
      target_->turn_off();
      break;
    case TOGGLE:
      target_->toggle();
      break;
    default:
      return;
  }
}

// TEXT_SENSOR actions

size_t I2CIDFSlaveDevice::I2CActionTextSensor_::get(TargetProperty property, uint8_t *data, const size_t max_size) {
  if (!target_->has_state())
    return 0;
  std::string state = target_->state;
  size_t size = (state.length() < max_size) ? state.length() : max_size;
  for (int i = 0; i < size; i++)
    data[i] = state[i];
  return size;
}

void I2CIDFSlaveDevice::I2CActionTextSensor_::set(TargetProperty property, int16_t value, bool relative) {
  std::string new_state = (const char *) &value;
  if (relative && target_->has_state()) {
    new_state = target_->state.append((const char *) &value);
    target_->publish_state(new_state);
    return;
  }
  target_->publish_state(new_state);
}

// Dump config to config log
void I2CIDFSlaveDevice::dump_config() {
  char *prefix_string = new char[prefix_size_ * 5];
  get_hex_string(prefix_string, prefix_, prefix_size_);

  ESP_LOGCONFIG(TAG, "I2C Slave Device :");
  ESP_LOGCONFIG(TAG, "  SDA Pin: %d", i2c_slave_config_.sda_io_num);
  ESP_LOGCONFIG(TAG, "    Pullup: %s", (i2c_slave_config_.sda_pullup_en == GPIO_PULLUP_ENABLE) ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  SCL Pin: %d", i2c_slave_config_.scl_io_num);
  ESP_LOGCONFIG(TAG, "    Pullup: %s", (i2c_slave_config_.scl_pullup_en == GPIO_PULLUP_ENABLE) ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02x", i2c_slave_config_.slave.slave_addr);
  ESP_LOGCONFIG(TAG, "  Max Frequency: %d %s",
                (i2c_slave_config_.slave.maximum_speed > 1000) ? i2c_slave_config_.slave.maximum_speed / 1000
                                                               : i2c_slave_config_.slave.maximum_speed,
                (i2c_slave_config_.slave.maximum_speed > 1000) ? "kHz" : "Hz");
  ESP_LOGCONFIG(TAG, "  Rx Buffer: %d commands, %d bytes", rx_buffer_size_, data_received_size_);
  ESP_LOGCONFIG(TAG, "  Tx Buffer: %d bytes", data_sent_size_);
  ESP_LOGCONFIG(TAG, "  ---  Command format - size: %d  --- ", command_size_);
  ESP_LOGCONFIG(TAG, "  Format ALL : %s <targetID> <action>", prefix_string);
  ESP_LOGCONFIG(TAG, "  Format GET : %s <targetID> <action> <property>", prefix_string);
  ESP_LOGCONFIG(TAG, "  Format SET : %s <targetID> <action> <property> <value>", prefix_string);
  ESP_LOGCONFIG(TAG, "  --- Target IDs --- ");
  for (const auto &[target_id, action] : actions_)
    ESP_LOGCONFIG(TAG, "  0x%02x   : %s", target_id, action->target_name.c_str());
  ESP_LOGCONFIG(TAG, "  ---  Actions   --- ");
  for (int i = 1; i < INVALID_ACTION && i < sizeof(str_target_actions) * sizeof(char *); i++)
    ESP_LOGCONFIG(TAG, "  0x%02x   : %s", i, str_target_actions[i]);
  ESP_LOGCONFIG(TAG, "  --- Properties --- ");
  for (int i = 1; i < INVALID_PROPERTY && i < sizeof(str_target_actions) * sizeof(char *); i++)
    ESP_LOGCONFIG(TAG, "  0x%02x   : %s", i, str_target_properties[i]);

  delete[] prefix_string;
}

// Destructor: clean up dynamically allocated buffers
I2CIDFSlaveDevice::~I2CIDFSlaveDevice() {
  if (ready_)
    i2c_driver_delete(i2c_slave_port_);
  for (const auto &[target_id, action] : actions_)
    if (action)
      delete action;
  if (prefix_)
    delete[] prefix_;
  if (data_received_)
    delete[] data_received_;
  if (data_sent_)
    delete[] data_sent_;
}

}  // namespace i2c_slave_device
}  // namespace esphome