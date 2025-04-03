
#include "i2c_slave_esp_idf.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace i2c_slave {

static const char *const TAG = "i2c_slave";

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

// CALL BACKS

#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2

bool IDFI2CSlave::i2c_slave_request_callback(i2c_slave_dev_handle_t i2c_slave,
                                             const i2c_slave_request_event_data_t *evt_data, void *arg) {
  uint32_t write_len;
  const uint8_t data_zero = 0x00;
  esp_err_t err = i2c_slave_write(i2c_slave, &data_zero, 1, &write_len, 0);
  return 0;
}

bool IDFI2CSlave::i2c_slave_receive_callback(i2c_slave_dev_handle_t i2c_slave,
                                             const i2c_slave_rx_done_event_data_t *evt_data, void *arg) {
  if (!evt_data)
    return 0;

  IDFI2CSlave *slave_dev = (IDFI2CSlave *) arg;
  uint32_t max_size = (evt_data->length < slave_dev->rx_buffer_size_) ? evt_data->length : slave_dev->rx_buffer_size_;

  for (int i = 0; i < max_size; i++)
    slave_dev->rx_buffer_[i] = evt_data->buffer[i];

  return 0;
}

#endif

// SETUP

void IDFI2CSlave::setup() {
  ESP_LOGI(TAG, "Setting up I2C Slave Bus...");
  esp_err_t err;

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

#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2

  // Set the update interval to 'never' as updates will rely on interrupt callback when using v2 driver
  this->set_update_interval(SCHEDULER_DONT_RUN);

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

  if (err == ESP_OK) {
    // Assign the callback functions
    i2c_slave_event_callbacks_t i2c_slave_event_callbacks{};
    memset(&i2c_slave_event_callbacks, 0, sizeof(i2c_slave_event_callbacks));
    i2c_slave_event_callbacks.on_request = i2c_slave_request_callback;
    i2c_slave_event_callbacks.on_receive = i2c_slave_receive_callback;

    err = i2c_slave_register_event_callbacks(this->i2c_slave_dev_handle_, &i2c_slave_event_callbacks, this);
  }

#else  // !CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2

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

#endif  // CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2

  if (err != ESP_OK) {
    // Fail the component if the I2C slave drive installation returned errors
    ESP_LOGE(TAG, "Failed to install I2C slave driver. Error: %d", err);
    mark_failed();
  } else {
    // Allocate local read and write buffers
    initialize_byte_buffer(this->rx_buffer_, this->rx_buffer_size_);
    initialize_byte_buffer(this->tx_buffer_, this->tx_buffer_size_);

    ESP_LOGI(TAG, "I2C Slave ready to receive on address 0x%02x", this->address_);
    ready_ = true;
  }
}

// UPDATE - COMMAND PROCESSING

void IDFI2CSlave::update() {
  // Read the rx buffers of the i2c device for new commands from master
  int rx_data_size = read_data();

  // Stop processing if data read size is smaller than minimum command size
  if (rx_data_size < 1)
    return;

  // Loop through the received data
  for (int i = 0; i <= rx_data_size; i++) {
  }
}

// I2C OPERATIONS

// Reads data sent by the master from the i2c rx buffer into the rx_buffer_ buffer and return the size
int IDFI2CSlave::read_data(size_t size) {
  if (!ready_)
    return -2;
  if (!size || size > this->rx_buffer_size_)
    size = this->rx_buffer_size_;
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  return size;
#else
  return i2c_slave_read_buffer(this->i2c_slave_port_, this->rx_buffer_, size, 0);
#endif
}

// Reads data sent by the master from the i2c rx buffer into a byte-array
void IDFI2CSlave::get_data(uint8_t *data, size_t size) {
  if (!data)
    return;
  int rx_data_size = this->read_data(size);
  for (int i = 0; i < rx_data_size && i < sizeof(data); i++)
    data[i] = this->rx_buffer_[i];
}

// Reads data sent by the master from the i2c rx buffer into a string
std::string IDFI2CSlave::get_data(size_t size) {
  int rx_data_size = this->read_data(size);

  if (rx_data_size == -2)
    return "NOT READY";

  if (rx_data_size < 0)
    return "ERROR";

  std::string received_data = "";
  for (int i = 0; i < rx_data_size; i++)
    received_data += this->rx_buffer_[i];

  return received_data;
}

// Writes byte-array to the i2c tx buffer to be read by master
int IDFI2CSlave::write_data(size_t size) {
  if (!ready_)
    return -2;
  if (!size)
    size = this->tx_buffer_size_;
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  uint32_t write_len = 0;
  esp_err_t err = i2c_slave_write(this->i2c_slave_dev_handle_, this->tx_buffer_, size, &write_len, 0);
  if (err)
    return -1;
  return (int) write_len;
#else
  return i2c_slave_write_buffer(this->i2c_slave_port_, this->tx_buffer_, size, 0);
#endif
}

// Writes string to the i2c tx buffer to be read by master
int IDFI2CSlave::write_data(std::string data) {
  size_t size = (data.length() < this->tx_buffer_size_) ? data.length() : this->tx_buffer_size_;
  for (int i = 0; i < size; i++)
    this->tx_buffer_[i] = data[i];
  return this->write_data(size);
}

// Dump config to config log
void IDFI2CSlave::dump_config() {
  const char *const driver_names[] = {"ARDUINO", "ESP-IDFv1", "ESP-IDFv2"};
  ESP_LOGCONFIG(TAG, "I2C Slave Device :");
  ESP_LOGCONFIG(TAG, "  SDA Pin: %d", this->sda_pin_);
  ESP_LOGCONFIG(TAG, "  SCL Pin: %d", this->scl_pin_);
  ESP_LOGCONFIG(TAG, "  Pullup: %s", (this->pullup_) ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02x", this->address_);
  ESP_LOGCONFIG(TAG, "  Rx Buffer: %d bytes", this->rx_buffer_size_);
  ESP_LOGCONFIG(TAG, "  Tx Buffer: %d bytes", this->tx_buffer_size_);
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  ESP_LOGCONFIG(TAG, "  Driver   : ESP-IDFv2");
#elif !CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
  ESP_LOGCONFIG(TAG, "  Driver   : ESP-IDFv1");
#else
  ESP_LOGCONFIG(TAG, "  Driver   : Arduino");
#endif
}

// Destructor: clean up dynamically allocated buffers
IDFI2CSlave::~IDFI2CSlave() {
  if (ready_)
#if CONFIG_I2C_ENABLE_SLAVE_DRIVER_VERSION_2
    esp_err_t err = i2c_del_slave_device(this->i2c_slave_dev_handle_);
#else
    esp_err_t err = i2c_driver_delete(this->i2c_slave_port_);
#endif
  if (this->rx_buffer_)
    delete[] this->rx_buffer_;
  if (this->tx_buffer_)
    delete[] this->tx_buffer_;
}

}  // namespace i2c_slave
}  // namespace esphome