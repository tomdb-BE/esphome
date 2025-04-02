#include "i2c_idf_slave_device.h"

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace i2c_slave_device {

static const char *const TAG = "i2c_slave_device";

// HELPERS

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
static std::string get_hex_string(const uint8_t *byte_array, const size_t size) {
  std::string hex_string = "";
  for (size_t i = 0; i < size; i++) {
    char byte_temp_string[5]{0};
    sprintf(byte_temp_string, "0x%02x ", byte_array[i]);
    hex_string = hex_string + byte_temp_string;
  }
  return hex_string;
}

#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
// CALL BACKS

// Call back on receive data from mastyer
bool I2CIDFSlaveDevice::i2c_slave_rx_callback(i2c_slave_dev_handle_t i2c_slave,
                                              const i2c_slave_rx_done_event_data_t *rx_event_data, void *arg) {
  ESP_LOGI(TAG, "Data received callback triggered");
  // i2c_slave_event_t evt = I2C_SLAVE_EVT_RX;
  // BaseType_t xTaskWoken = 0;
  I2CIDFSlaveDevice *slave_device = (I2CIDFSlaveDevice *) arg;
  if (slave_device)
    slave_device->handle_rx_event(rx_event_data);
  // xQueueSendFromISR(context->event_queue, &evt, &xTaskWoken);
  return 0;
}

void I2CIDFSlaveDevice::handle_rx_event(const i2c_slave_rx_done_event_data_t *rx_buffer) {
  if (!rx_buffer)
    return;

  uint32_t max_size = (rx_buffer->length < this->data_received_size_) ? rx_buffer->length : this->data_received_size_;

  for (int i = 0; i < max_size; i++)
    this->data_received_[i] = rx_buffer->buffer[i];
}

#endif

// SETUP

void I2CIDFSlaveDevice::setup() {
  ESP_LOGI(TAG, "Setting up I2C Slave Bus...");
  esp_err_t err;

  // Calculate the total command size
  this->command_size_ = this->prefix_size_ + 2;
  this->data_received_size_ = this->command_size_ * this->rx_buffer_size_;
  this->data_sent_size_ = this->tx_buffer_size_;

  // tx and rx buffer size needs to be > 100 for I2C esp-idf driver in slave mode
  size_t internal_rx_buffer_size = (this->data_received_size_ < 100) ? 100 : this->data_received_size_;
  size_t internal_tx_buffer_size = (this->data_sent_size_ < 100) ? 100 : this->data_sent_size_;

  // Find and set free I2C port
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

// Set the i2c driver config and start it
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2

  uint32_t internal_pullup = (this->pullup_) ? 1 : 0;
  i2c_slave_config_t i2c_slave_config = {
      .i2c_port = this->i2c_slave_port_,
      .sda_io_num = this->sda_pin_,
      .scl_io_num = this->scl_pin_,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .send_buf_depth = 100,
      .receive_buf_depth = 100,
      .slave_addr = this->address_,
      .addr_bit_len = I2C_ADDR_BIT_LEN_7,
      .intr_priority = 0,
      .flags =
          {
              .allow_pd = 1,
              .enable_internal_pullup = internal_pullup,
          },
  };

  // Register the i2c slave device
  err = i2c_new_slave_device(&i2c_slave_config, &this->i2c_slave_dev_handle_);

  // Assign the callback function on receive and send
  i2c_slave_event_callbacks_t i2c_slave_callbacks = {
      .on_receive = i2c_slave_rx_callback,
  };
  err = i2c_slave_register_event_callbacks(this->i2c_slave_dev_handle_, &i2c_slave_callbacks, this);

  // Set the update interval to 'never' as we will use callbacks in driver v2
  this->set_update_interval(SCHEDULER_DONT_RUN);

#else

  i2c_config_t i2c_slave_config{};
  memset(&i2c_slave_config, 0, sizeof(i2c_slave_config));
  i2c_slave_config.mode = I2C_MODE_SLAVE;
  i2c_slave_config.sda_io_num = this->sda_pin_;
  i2c_slave_config.scl_io_num = this->scl_pin_;
  i2c_slave_config.sda_pullup_en = this->pullup_;
  i2c_slave_config.scl_pullup_en = this->pullup_;
  i2c_slave_config.slave.slave_addr = this->address_;

  err = i2c_param_config(this->i2c_slave_port_, &i2c_slave_config);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure I2C slave on port %d. Error: %d", i2c_slave_port_, err);
    mark_failed();
    return;
  }

  err = i2c_driver_install(this->i2c_slave_port_, I2C_MODE_SLAVE, internal_rx_buffer_size, internal_tx_buffer_size, 0);

#endif

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to install I2C slave driver. Error: %d", err);
    mark_failed();
    return;
  }

  // Allocate buffers
  initialize_byte_buffer(this->data_received_, this->data_received_size_);
  initialize_byte_buffer(this->data_sent_, this->data_sent_size_);

  ESP_LOGI(TAG, "I2C Slave ready to receive on address 0x%02x", this->address_);
  ready_ = true;
}

// Setup the prefix
void I2CIDFSlaveDevice::set_prefix(uint32_t prefix) {
  size_t size = 0;
  for (int i = 3; i >= 0; i--) {
    uint8_t byte_value = (prefix >> i * 8) & 0xFF;
    // Set the size when first most significant byte != 0
    if (!size && byte_value)
      size = i + 1;
    this->prefix_[i] = byte_value;
  }
  this->prefix_size_ = size;
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
  if (rx_data_size < this->command_size_)
    return;

  // Loop through the received data
  for (int i = 0; i <= rx_data_size - this->command_size_; i++) {
    // Check if the prefix is valid
    if (prefix_size_ > 0) {
      bool prefix_valid = true;
      for (int j = 0; j < this->prefix_size_; j++) {
        if (this->prefix_[j] != this->data_received_[j + i]) {
          prefix_valid = false;
          ESP_LOGD(TAG, "Invalid prefix byte: 0x%02x, expected: 0x%02x", this->data_received_[j + i], this->prefix_[i]);
          break;
        }
        i++;
      }
      if (!prefix_valid)
        continue;
    }

    // Check if the target component is configured
    uint8_t target = this->data_received_[i];
    if (!this->actions_[target]) {
      ESP_LOGD(TAG, "Invalid target ID: 0x%02x", target);
      continue;
    }

    // Check if the requested action type is valid
    if (!this->data_received_[i + 1] || this->data_received_[i + 1] >= INVALID_ACTION) {
      ESP_LOGD(TAG, "Invalid action: 0x%02x", this->data_received_[i + 1]);
      continue;
    }
    i++;
    TargetAction action = (TargetAction) this->data_received_[i];

    // Trigger the action if no property and/or value is required. (not a GET/SET action)
    if (action < GET) {
      this->actions_[target]->trigger(action);
      ESP_LOGD(TAG, "Target ID 0x%02x: triggered action %s", target, str_target_actions[action]);
      continue;
    }

    // Check if the property to SET/GET is valid, if not, default to SET/GET the state
    TargetProperty property = STATE;
    if (i + 1 < rx_data_size && this->data_received_[i + 1] && this->data_received_[i + 1] < INVALID_PROPERTY) {
      i++;
      property = (TargetProperty) this->data_received_[i];
    }

    // Get the requested property from the component and write it to the tx buffer so the master can read it
    if (action == GET) {
      size_t data_size = this->actions_[target]->get(property, this->data_sent_, this->data_sent_size_);
      write_data(data_size);
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
    int16_t value = (int16_t) this->data_received_[i];
    // Set the relative flag if the action is INCREASE or DECREASE
    bool relative = (action == INCREASE || action == DECREASE);
    // Set the relative value to a negative number when the action is DECREASE
    if (action == DECREASE)
      value = 0 - value;
    // Trigger the action on the defined property with the defined value
    this->actions_[target]->set(property, value, relative);
    ESP_LOGD(TAG, "Target ID 0x%02x: SET %s with %s value %d", target, str_target_properties[property],
             (relative) ? "relative" : "absolute", value);
  }
}

// I2C OPERATIONS

// Reads data sent by the master from the i2c rx buffer into the data_received_ buffer and return the size
int I2CIDFSlaveDevice::read_data(size_t size) {
  if (!ready_)
    return -2;
  if (!size || size > this->data_received_size_)
    size = this->data_received_size_;
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  return 0;
#else
  return i2c_slave_read_buffer(this->i2c_slave_port_, this->data_received_, size, 0);
#endif
}

// Reads data sent by the master from the i2c rx buffer into a byte-array
void I2CIDFSlaveDevice::get_data(uint8_t *data, size_t size) {
  if (!data)
    return;
  int rx_data_size = this->read_data(size);
  for (int i = 0; i < rx_data_size && i < sizeof(data); i++)
    data[i] = this->data_received_[i];
}

// Reads data sent by the master from the i2c rx buffer into a string
std::string I2CIDFSlaveDevice::get_data(size_t size) {
  int rx_data_size = this->read_data(size);

  if (rx_data_size == -2)
    return "NOT READY";

  if (rx_data_size < 0)
    return "ERROR";

  std::string received_data = "";
  for (int i = 0; i < rx_data_size; i++)
    received_data += this->data_received_[i];

  return received_data;
}

// Writes byte-array to the i2c tx buffer to be read by master
int I2CIDFSlaveDevice::write_data(size_t size) {
  if (!ready_)
    return -2;
  if (!size)
    size = this->data_sent_size_;
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  return 0;
#else
  return i2c_slave_write_buffer(this->i2c_slave_port_, this->data_sent_, size, 0);
#endif
}

// Writes string to the i2c tx buffer to be read by master
int I2CIDFSlaveDevice::write_data(std::string data) {
  size_t size = (data.length() < this->data_sent_size_) ? data.length() : this->data_sent_size_;
  for (int i = 0; i < size; i++)
    this->data_sent_[i] = data[i];
  return this->write_data(size);
}

// ACTION HANDLERS

// COVER actions

size_t I2CIDFSlaveDevice::I2CActionCover_::get(TargetProperty property, uint8_t *data, const size_t max_size) {
  switch (property) {
    case STATE:
      data[0] = (this->target_->position > 0.0f) ? 1 : 0;
      break;
    case POSITION:
      data[0] = percent_float_to_byte(this->target_->position);
      break;
    case TILT:
      data[0] = percent_float_to_byte(this->target_->tilt);
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
        float old_value = percent_float_to_byte(this->target_->position);
        get_diff_value(&old_value, &new_value);
      }
      this->target_->make_call().set_position(new_value).perform();
      break;
    case TILT:
      if (relative) {
        float old_value = percent_float_to_byte(this->target_->tilt);
        get_diff_value(&old_value, &new_value);
      }
      this->target_->make_call().set_tilt(new_value).perform();
      break;
    default:
      return;
  }
}

void I2CIDFSlaveDevice::I2CActionCover_::trigger(TargetAction action) {
  switch (action) {
    case OPEN:
      this->target_->make_call().set_command_open().perform();
      break;
    case CLOSE:
      this->target_->make_call().set_command_close().perform();
      break;
    case TOGGLE:
      this->target_->make_call().set_command_toggle().perform();
      break;
    case STOP:
      this->target_->make_call().set_command_stop().perform();
      break;
    default:
      return;
  }
}

// LIGHT actions

size_t I2CIDFSlaveDevice::I2CActionLight_::get(TargetProperty property, uint8_t *data, const size_t max_size) {
  switch (property) {
    case BRIGHTNESS:
      data[0] = percent_float_to_byte(this->target_->current_values.get_brightness());
      break;
    case RED:
      data[0] = percent_float_to_byte(this->target_->current_values.get_red());
      break;
    case GREEN:
      data[0] = percent_float_to_byte(this->target_->current_values.get_green());
      break;
    case BLUE:
      data[0] = percent_float_to_byte(this->target_->current_values.get_blue());
      break;
    case WHITE:
      data[0] = percent_float_to_byte(this->target_->current_values.get_white());
      break;
    case COLD_WHITE:
      data[0] = percent_float_to_byte(this->target_->current_values.get_cold_white());
      break;
    case WARM_WHITE:
      data[0] = percent_float_to_byte(this->target_->current_values.get_warm_white());
      break;
    case COLOR_TEMPERATURE:
      data[0] = percent_float_to_byte(this->target_->current_values.get_color_temperature());
      break;
    case EFFECT: {
      if (!this->target_->supports_effects())
        return 0;
      std::string effect = this->target_->get_effect_name();
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
        float old_value = this->target_->current_values.get_brightness();
        get_diff_value(&old_value, &new_value);
      }
      this->target_->make_call().set_brightness_if_supported(new_value).perform();
      break;
    case RED:
      if (relative) {
        float old_value = this->target_->current_values.get_red();
        get_diff_value(&old_value, &new_value);
      }
      this->target_->make_call().set_red_if_supported(new_value).perform();
      break;
    case GREEN:
      if (relative) {
        float old_value = this->target_->current_values.get_green();
        get_diff_value(&old_value, &new_value);
      }
      this->target_->make_call().set_green_if_supported(new_value).perform();
      break;
    case BLUE:
      if (relative) {
        float old_value = this->target_->current_values.get_blue();
        get_diff_value(&old_value, &new_value);
      }
      this->target_->make_call().set_blue_if_supported(new_value).perform();
      break;
    case WHITE:
      if (relative) {
        float old_value = this->target_->current_values.get_white();
        get_diff_value(&old_value, &new_value);
      }
      this->target_->make_call().set_white_if_supported(new_value).perform();
      break;
    case COLD_WHITE:
      if (relative) {
        float old_value = this->target_->current_values.get_cold_white();
        get_diff_value(&old_value, &new_value);
      }
      this->target_->make_call().set_cold_white_if_supported(new_value).perform();
      break;
    case WARM_WHITE:
      if (relative) {
        float old_value = this->target_->current_values.get_warm_white();
        get_diff_value(&old_value, &new_value);
      }
      this->target_->make_call().set_warm_white_if_supported(new_value).perform();
      break;
    case COLOR_TEMPERATURE:
      if (relative) {
        float old_value = this->target_->current_values.get_color_temperature();
        get_diff_value(&old_value, &new_value);
      }
      this->target_->make_call().set_color_temperature_if_supported(new_value).perform();
      break;
    case EFFECT:
      if (!this->target_->supports_effects())
        return;
      this->target_->make_call().set_effect((uint32_t) value).perform();
      break;
    default:
      return;
  }
}

void I2CIDFSlaveDevice::I2CActionLight_::trigger(TargetAction action) {
  switch (action) {
    case TURN_ON:
      this->target_->turn_on().perform();
      break;
    case TURN_OFF:
      this->target_->turn_off().perform();
      break;
    case TOGGLE:
      this->target_->toggle().perform();
      break;
    default:
      return;
  }
}

// SENSOR actions

size_t I2CIDFSlaveDevice::I2CActionSensor_::get(TargetProperty property, uint8_t *data, const size_t max_size) {
  size_t size = sizeof(float);
  uint8_t *state = (uint8_t *) &this->target_->state;
  for (int i = size - 1; i >= 0; i--) {
    data[i] = state[i];
  }
  return size;
}

void I2CIDFSlaveDevice::I2CActionSwitch_::trigger(TargetAction action) {
  switch (action) {
    case TURN_ON:
      this->target_->turn_on();
      break;
    case TURN_OFF:
      this->target_->turn_off();
      break;
    case TOGGLE:
      this->target_->toggle();
      break;
    default:
      return;
  }
}

// TEXT_SENSOR actions

size_t I2CIDFSlaveDevice::I2CActionTextSensor_::get(TargetProperty property, uint8_t *data, const size_t max_size) {
  if (!this->target_->has_state())
    return 0;
  std::string state = this->target_->state;
  size_t size = (state.length() < max_size) ? state.length() : max_size;
  for (int i = 0; i < size; i++)
    data[i] = state[i];
  return size;
}

void I2CIDFSlaveDevice::I2CActionTextSensor_::set(TargetProperty property, int16_t value, bool relative) {
  std::string new_state = (const char *) &value;
  if (relative && this->target_->has_state()) {
    new_state = this->target_->state.append((const char *) &value);
    this->target_->publish_state(new_state);
    return;
  }
  this->target_->publish_state(new_state);
}

// Dump config to config log
void I2CIDFSlaveDevice::dump_config() {
  std::string prefix_string = get_hex_string(this->prefix_, this->prefix_size_);

  ESP_LOGCONFIG(TAG, "I2C Slave Device :");
  ESP_LOGCONFIG(TAG, "  SDA Pin: %d", this->sda_pin_);
  ESP_LOGCONFIG(TAG, "  SCL Pin: %d", this->scl_pin_);
  ESP_LOGCONFIG(TAG, "  Pullup: %s", (this->pullup_) ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02x", this->address_);
  ESP_LOGCONFIG(TAG, "  Rx Buffer: %d commands, %d bytes", this->rx_buffer_size_);
  ESP_LOGCONFIG(TAG, "  Tx Buffer: %d bytes", this->tx_buffer_size_);
  ESP_LOGCONFIG(TAG, "  Prefix   : %s", prefix_string.c_str());
  ESP_LOGCONFIG(TAG, "  ---  Command format - size: %d  --- ", this->command_size_);
  ESP_LOGCONFIG(TAG, "  Format ALL : %s <targetID> <action>", prefix_string.c_str());
  ESP_LOGCONFIG(TAG, "  Format GET : %s <targetID> <action> <property>", prefix_string.c_str());
  ESP_LOGCONFIG(TAG, "  Format SET : %s <targetID> <action> <property> <value>", prefix_string.c_str());
  ESP_LOGCONFIG(TAG, "  --- Target IDs --- ");
  for (const auto &[target_id, action] : this->actions_)
    ESP_LOGCONFIG(TAG, "  0x%02x   : %s", target_id, action->target_name.c_str());
  ESP_LOGCONFIG(TAG, "  ---  Actions   --- ");
  for (int i = 1; i < INVALID_ACTION && i < sizeof(str_target_actions) * sizeof(char *); i++)
    ESP_LOGCONFIG(TAG, "  0x%02x   : %s", i, str_target_actions[i]);
  ESP_LOGCONFIG(TAG, "  --- Properties --- ");
  for (int i = 1; i < INVALID_PROPERTY && i < sizeof(str_target_actions) * sizeof(char *); i++)
    ESP_LOGCONFIG(TAG, "  0x%02x   : %s", i, str_target_properties[i]);
}

// Destructor: clean up dynamically allocated buffers
I2CIDFSlaveDevice::~I2CIDFSlaveDevice() {
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
#else
  if (ready_)
    i2c_driver_delete(this->i2c_slave_port_);
#endif
  for (const auto &[target_id, action] : this->actions_) {
    if (action)
      delete action;
  }
  if (this->data_received_)
    delete[] this->data_received_;
  if (this->data_sent_)
    delete[] this->data_sent_;
}

}  // namespace i2c_slave_device
}  // namespace esphome