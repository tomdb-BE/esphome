import logging

import esphome.codegen as cg
from esphome.components.i2c import pin_with_input_and_output_support
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_ID,
    CONF_PULLUP,
    CONF_SCL,
    CONF_SDA,
    PLATFORM_ESP32,
    PLATFORM_ESP8266,
    PLATFORM_RP2040,
)
from esphome.core import CORE, coroutine_with_priority

CONF_RX_BUFFER_SIZE = "receive_buffer"
CONF_TX_BUFFER_SIZE = "send_buffer"

CODEOWNERS = ["@tomdb-be"]

i2c_slave_ns = cg.esphome_ns.namespace("i2c_slave")
IDFI2CSlave = i2c_slave_ns.class_("IDFI2CSlave", cg.Component)


def _framework_declare_type(value):
    if CORE.using_esp_idf:
        return cv.declare_id(IDFI2CSlave)(value)
    raise NotImplementedError


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): _framework_declare_type,
            cv.Optional(CONF_SDA, default="SDA"): pin_with_input_and_output_support,
            cv.Optional(CONF_SCL, default="SDL"): pin_with_input_and_output_support,
            cv.SplitDefault(CONF_PULLUP, esp32_idf=True): cv.All(
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
    cv.only_on([PLATFORM_ESP32, PLATFORM_ESP8266, PLATFORM_RP2040]),
    cv.only_with_esp_idf,
)


@coroutine_with_priority(1.0)
async def to_code(config):
    logging.warning(config)

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_sda_pin(config[CONF_SDA]))
    cg.add(var.set_scl_pin(config[CONF_SCL]))
    cg.add(var.set_pullup(config[CONF_PULLUP]))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_rx_buffer_size(config[CONF_RX_BUFFER_SIZE]))
    cg.add(var.set_tx_buffer_size(config[CONF_TX_BUFFER_SIZE]))

    if CORE.using_arduino:
        cg.add_library("Wire", None)
