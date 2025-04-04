import logging

import esphome.codegen as cg
from esphome.components.i2c import pin_with_input_and_output_support
import esphome.config_validation as cv
from esphome.const import CONF_ADDRESS, CONF_ID, CONF_PULLUP, CONF_SCL, CONF_SDA
from esphome.core import CORE, coroutine_with_priority

CODEOWNERS = ["@tomdb-be"]

CONF_RX_BUFFER_SIZE = "receive_buffer"
CONF_TX_BUFFER_SIZE = "send_buffer"

MULTI_CONF = True

i2c_slave_device_ns = cg.esphome_ns.namespace("i2c_slave_device")
I2CSlaveDevice = i2c_slave_device_ns.class_("I2CSlaveDevice", cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(I2CSlaveDevice),
            cv.Optional(CONF_SDA, default="SDA"): pin_with_input_and_output_support,
            cv.Optional(CONF_SCL, default="SDL"): pin_with_input_and_output_support,
            cv.SplitDefault(CONF_PULLUP, default=True): cv.All(
                cv.only_with_esp_idf, cv.boolean
            ),
            cv.Optional(CONF_ADDRESS, default=0x08): cv.hex_uint8_t,
            cv.Optional(CONF_RX_BUFFER_SIZE, default=101): cv.All(
                cv.uint16_t, cv.Range(min=101)
            ),
            cv.Optional(CONF_TX_BUFFER_SIZE, default=101): cv.All(
                cv.uint16_t, cv.Range(min=101)
            ),
        }
    ).extend(cv.polling_component_schema("10s")),
)


@coroutine_with_priority(1.0)
async def to_code(config):
    logging.warning(config)

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_sda_pin(config[CONF_SDA]))
    cg.add(var.set_scl_pin(config[CONF_SCL]))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_rx_buffer_size(config[CONF_RX_BUFFER_SIZE]))
    cg.add(var.set_tx_buffer_size(config[CONF_TX_BUFFER_SIZE]))

    if CORE.using_arduino:
        cg.add_library("Wire", None)
    else:
        cg.add(var.set_pullup(config[CONF_PULLUP]))
