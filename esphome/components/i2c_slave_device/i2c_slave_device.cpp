#include "i2c_slave_device.h"

#include "esphome/core/log.h"

namespace esphome {
namespace i2c_slave_device {

static const char *const TAG = "i2c_slave";

// HELPERS

// Create a new buffer with specified size and set contents to zero. Delete if already exists.
static void initialize_byte_buffer(uint8_t *&byte_array, uint32_t size) {
  if (byte_array)
    delete[] byte_array;
  byte_array = new uint8_t[size];
  for (int i = 0; i < size; i++)
    byte_array[i] = 0;
  return;
}

// CALL BACKS

// ESP-IDF I2C Slave Driver v2 Callbacks
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2

// On request callback function. The tx_buffer of the individual instance is accessible via the pointer in arg.
bool I2CSlaveDevice::i2c_slave_tx_callback(i2c_slave_dev_handle_t i2c_slave_handle,
                                           const i2c_slave_request_event_data_t *evt_data, void *arg) {
  if (arg == nullptr || !i2c_slave_handle)
    return 0;

  uint32_t write_size = 0;
  I2CSlaveDevice *slave_dev = static_cast<I2CSlaveDevice *>(arg);

  if (slave_dev && slave_dev->new_tx_size_ > 0) {
    esp_err_t err = i2c_slave_write(i2c_slave_handle, slave_dev->tx_buffer_, slave_dev->new_tx_size_, &write_size, 0);
    if (err == ESP_OK) {
      slave_dev->last_tx_size_ = write_size;
      slave_dev->new_tx_size_ = 0;
    }
  } else {
    // Respond to the write request with a 0 set buffer if there is no new data to send.
    // The master expects a response even if there is no new data.
    const uint8_t buf_zero[] = {0x00};
    esp_err_t err = i2c_slave_write(i2c_slave_handle, buf_zero, 1, &write_size, 0);
  }
  return 0;
}

// On receive callback function. The rx_buffer of the individual instance is accessible via the pointer in arg.
bool I2CSlaveDevice::i2c_slave_rx_callback(i2c_slave_dev_handle_t i2c_slave_handle,
                                           const i2c_slave_rx_done_event_data_t *evt_data, void *arg) {
  if (arg == nullptr || !evt_data || !evt_data->length)
    return 0;

  I2CSlaveDevice *slave_dev = static_cast<I2CSlaveDevice *>(arg);
  if (!slave_dev)
    return 0;

  uint32_t size = (evt_data->length < slave_dev->rx_buffer_size_) ? evt_data->length : slave_dev->rx_buffer_size_;

  for (int i = 0; i < size; i++)
    slave_dev->rx_buffer_[i] = evt_data->buffer[i];

  slave_dev->new_rx_size_ = size;
  slave_dev->last_rx_size_ = size;

  return 0;
}

#endif  // ESP-IDF I2C Slave Driver v2 Callbacks

// SETUP

void I2CSlaveDevice::setup() {
  int err = 0;

#if defined(USE_ARDUINO)  // Arduino Framework

// Find free port and initiate I2C
#if defined(USE_ESP32)
  ESP_LOGI(TAG, "Setting up I2C Slave using Arduino-ESP32...");
  static uint8_t next_bus_num = 0;
  if (next_bus_num < I2C_NUM_MAX)
    this->wire_ = (next_bus_num == 0) ? &Wire : new TwoWire(next_bus_num);
  next_bus_num++;
#elif defined(USE_ESP8266)
  ESP_LOGI(TAG, "Setting up I2C Slave using Arduino-ESP8266...");
  this->wire_ = new TwoWire();
#elif defined(USE_RP2040)
  ESP_LOGI(TAG, "Setting up I2C Slave using Arduino-RP2040...");
  static bool first = true;
  this->wire_ = (first) ? &Wire : &Wire1;
  first = false;
#endif

  if (!this->wire_) {
    ESP_LOGE(TAG, "Failed to create I2C device. Max %u supported.", I2C_NUM_MAX);
    this->mark_failed();
    return;
  }

  // Set callback functions
  this->wire_->onRequest(i2c_slave_tx_callback);
  this->wire_->onReceive(i2c_slave_rx_callback);

  // Start the driver
#if defined(USE_ESP32)
  uint32_t buffer_size =
      (this->rx_buffer_size_ > this->tx_buffer_size_) ? this->rx_buffer_size_ : this->tx_buffer_size_;
  this->wire_->setBufferSize(buffer_size);
  this->wire_->setPins(static_cast<int>(this->sda_pin_), static_cast<int>(this->scl_pin_));
  bool wire_started = this->wire_->begin(this->address_);
  err = (wire_started) ? ESP_OK : ESP_FAIL;
#elif defined(USE_ESP8266)
  this->wire_->begin(this->address_, static_cast<int>(this->sda_pin_), static_cast<int>(this->scl_pin_));
#elif defined(USE_RP2040)
  this->wire_->setSDA(this->sda_pin_);
  this->wire_->setSCL(this->scl_pin_);
  this->wire_->begin(this->address_);
#endif

#else  //  ESP-IDF Framework

  // Find and set free I2C port
  static i2c_port_t next_port = I2C_NUM_0;
  this->i2c_slave_port_ = next_port;
#if SOC_I2C_NUM > 1
  next_port = (next_port == I2C_NUM_0) ? I2C_NUM_1 : I2C_NUM_MAX;
#else
  next_port = I2C_NUM_MAX;
#endif
  if (this->i2c_slave_port_ == I2C_NUM_MAX) {
    ESP_LOGE(TAG, "Too many I2C buses configured. Max %u supported.", I2C_NUM_MAX);
    this->mark_failed();
    return;
  }

#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2  // ESP-IDF driver v2

  ESP_LOGI(TAG, "Setting up I2C Slave using ESP-IDF - driver v2...");

  // Set the update interval to 'never' as updates will rely on interrupt callback when using v2 driver
  // this->set_update_interval(SCHEDULER_DONT_RUN);

  // Set the I2C slave driver v2 config
  i2c_slave_config_t i2c_slave_config{};
  memset(&i2c_slave_config, 0, sizeof(i2c_slave_config));
  i2c_slave_config.i2c_port = this->i2c_slave_port_;
  i2c_slave_config.clk_source = I2C_CLK_SRC_DEFAULT;
  i2c_slave_config.sda_io_num = this->sda_pin_;
  i2c_slave_config.scl_io_num = this->scl_pin_;
  i2c_slave_config.send_buf_depth = this->tx_buffer_size_;
  i2c_slave_config.receive_buf_depth = this->tx_buffer_size_;
  i2c_slave_config.slave_addr = this->address_;
  i2c_slave_config.flags.enable_internal_pullup = this->pullup_;

  // Install the I2C slave driver v2
  err = i2c_new_slave_device(&i2c_slave_config, &this->i2c_slave_dev_handle_);

  if (err == 0) {
    // Assign the callback functions
    i2c_slave_event_callbacks_t i2c_slave_event_callbacks{};
    memset(&i2c_slave_event_callbacks, 0, sizeof(i2c_slave_event_callbacks));
    i2c_slave_event_callbacks.on_request = i2c_slave_tx_callback;
    i2c_slave_event_callbacks.on_receive = i2c_slave_rx_callback;

    err = i2c_slave_register_event_callbacks(this->i2c_slave_dev_handle_, &i2c_slave_event_callbacks, this);
  }

#else  // ESP-IDF driver v1

  ESP_LOGI(TAG, "Setting up I2C Slave using ESP-IDF - driver v1...");

  // Define the I2C slave driver v1 config
  i2c_config_t i2c_slave_config{};
  memset(&i2c_slave_config, 0, sizeof(i2c_slave_config));
  i2c_slave_config.mode = I2C_MODE_SLAVE;
  i2c_slave_config.sda_io_num = this->sda_pin_;
  i2c_slave_config.scl_io_num = this->scl_pin_;
  i2c_slave_config.sda_pullup_en = this->pullup_;
  i2c_slave_config.scl_pullup_en = this->pullup_;
  i2c_slave_config.slave.slave_addr = this->address_;
#ifdef USE_ESP32_VARIANT_ESP32S2
  // workaround for https://github.com/esphome/issues/issues/6718
  i2c_slave_config.clk_flags = I2C_SCLK_SRC_FLAG_AWARE_DFS;
#endif  // USE_ESP32_VARIANT_ESP32S2

  // Apply the I2C slave driver v1 config
  err = i2c_param_config(this->i2c_slave_port_, &i2c_slave_config);
  // Install the I2C slave driver v1
  if (err == ESP_OK)
    err = i2c_driver_install(this->i2c_slave_port_, I2C_MODE_SLAVE, this->rx_buffer_size_, this->tx_buffer_size_, 0);

#endif  // ESP-IDF driver

#endif  // USE_ARDUINO

  if (err != 0) {
    // Fail the component if the I2C slave drive installation returned errors
    ESP_LOGE(TAG, "Failed to install I2C slave driver. Error: %d", err);
    this->mark_failed();
  } else {
    // Allocate local read and write buffers
    initialize_byte_buffer(this->rx_buffer_, this->rx_buffer_size_);
    initialize_byte_buffer(this->tx_buffer_, this->tx_buffer_size_);

    ESP_LOGI(TAG, "I2C Slave ready to receive on address 0x%02x", this->address_);
    this->initialized_ = true;
  }
}

// I2C OPERATIONS

uint32_t I2CSlaveDevice::read_raw(uint8_t *rx_byte_array, uint32_t size) {
  // Return when pointer to target byte array is invalid or if no data has been received
  if (!rx_byte_array || this->last_rx_size_ < 1)
    return 0;
  // Set the requested size to the size of the data received when the provided size is not set or larger.
  if (!size || size > this->last_rx_size_)
    size = this->last_rx_size_;
  // Do not exceed the available size in the target byte array.
  if (size > sizeof(rx_byte_array))
    size = sizeof(rx_byte_array);
  // Copy <size> bytes from the rx_buffer to the target byte array.
  for (int i = 0; i < size; i++)
    rx_byte_array[i] = this->rx_buffer_[i];
  // Return the actual size read
  return size;
}

std::string I2CSlaveDevice::read(uint32_t size) {
  if (this->last_rx_size_ < 1)
    return "";
  // If no size is provided of if the size exceeds the size of data read, read all available data.
  if (!size || size > this->last_rx_size_)
    size = this->last_rx_size_;
  // Return the string
  return std::string(&this->rx_buffer_[0], &this->rx_buffer_[size]);
}

uint32_t I2CSlaveDevice::write(const uint8_t *tx_byte_array, uint32_t size) {
  // Return when I2C slave device not ready
  if (!initialized_)
    return 0;
  // Limit the chars written to the tx_buffer size
  if (size > this->tx_buffer_size_)
    size = this->tx_buffer_size_;
  // Write <size> characters to the tx buffer. The master can now request this data.
  for (int i = 0; i < size; i++)
    this->tx_buffer_[i] = tx_byte_array[i];
  this->new_tx_size_ = size;
#if !CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  this->set_tx_buffer_();
#endif
  return this->last_tx_size_;
}

uint32_t I2CSlaveDevice::write(const std::string tx_string, uint32_t size) {
  if (!size || size > tx_string.length())
    size = (uint32_t) tx_string.length();
  return this->write((uint8_t *) tx_string.c_str(), size);
}

// Copies the recieved data from master from the I2C's rx buffer
// ESP-IDF driver v2: this function is not called and and rx buffer is updated via callback on_receive
#if !CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2

void I2CSlaveDevice::get_rx_buffer_() {
  int rx_size = 0;
  if (!initialized_)
    return;

#if defined(USE_ARDUINO)
  // Return if no new data received from master
  if (!this->wire_->available())
    return;
  // Copies the I2C's internal rx buffer
  for (int i = 0; this->wire_->available() && i < this->rx_buffer_size_; i++) {
    this->rx_buffer_[i] = this->wire_->read();
    rx_size++;
  }
#else
  // ESP-IDF driver v1: Copies the I2C's internal rx buffer
  rx_size = i2c_slave_read_buffer(this->i2c_slave_port_, this->rx_buffer_, this->rx_buffer_size_, 0);
#endif
  if (rx_size > 0) {
    // const char * str_rx_new =
    this->last_rx_size_ = rx_size;
    this->on_rx_callback_.call(this->read(rx_size));
  }
}

// Copies the tx buffer to the I2C's internal tx buffer so the master can read it upon request
void I2CSlaveDevice::set_tx_buffer_() {
  // Return when I2C slave device not ready
  if (!initialized_)
    return;
#if defined(USE_ARDUINO)
#if defined(USE_ESP32)
  this->wire_->slaveWrite(this->tx_buffer_, this->new_tx_size_);
#else   // ESP8266 and RP2040
  this->wire_->write(this->tx_buffer_, this->new_tx_size_);
#endif  // USE_ESP32
#else   // ESP-IDF driver version 1
  int write_size = i2c_slave_write_buffer(this->i2c_slave_port_, this->tx_buffer_, this->new_tx_size_, 0);
  this->new_tx_size = (write_size > 0) ? write_size : 0;

#endif
  if (this->new_tx_size_ > 0) {
    this->last_tx_size_ = this->new_tx_size_;
    this->new_tx_size_ = 0;
  }
}

#endif

// Dump config to config log
void I2CSlaveDevice::dump_config() {
  ESP_LOGCONFIG(TAG, "I2C Slave Device :");
  ESP_LOGCONFIG(TAG, "  SDA Pin: GPIO%d", this->sda_pin_);
  ESP_LOGCONFIG(TAG, "  SCL Pin: GPIO%d", this->scl_pin_);
  ESP_LOGCONFIG(TAG, "  Pullup: %s", (this->pullup_) ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02x", this->address_);
  ESP_LOGCONFIG(TAG, "  Rx Buffer: %d bytes", this->rx_buffer_size_);
  ESP_LOGCONFIG(TAG, "  Tx Buffer: %d bytes", this->tx_buffer_size_);
#if defined(USE_ARDUINO)
  ESP_LOGCONFIG(TAG, "  Driver   : Arduino");
#elif CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  ESP_LOGCONFIG(TAG, "  Driver   : ESP-IDFv2");
#else
  ESP_LOGCONFIG(TAG, "  Driver   : ESP-IDFv1");
#endif
}

// Destructor: clean up dynamically allocated buffers
I2CSlaveDevice::~I2CSlaveDevice() {
  if (initialized_)
#if defined(USE_ARDUINO)
    if (this->wire_)
      delete this->wire_;
#elif CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
    esp_err_t err = i2c_del_slave_device(this->i2c_slave_dev_handle_);
#else
    esp_err_t err = i2c_driver_delete(this->i2c_slave_port_);
#endif
  if (this->rx_buffer_)
    delete[] this->rx_buffer_;
  if (this->tx_buffer_)
    delete[] this->tx_buffer_;
}

}  // namespace i2c_slave_device
}  // namespace esphome