import logging

import esphome.codegen as cg
from esphome.components.binary_sensor import BinarySensor
from esphome.components.button import Button
from esphome.components.cover import Cover
from esphome.components.i2c import pin_with_input_and_output_support
from esphome.components.light import LightState
from esphome.components.sensor import Sensor
from esphome.components.switch import Switch
from esphome.components.text_sensor import TextSensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_BINARY_SENSOR,
    CONF_BUTTON,
    CONF_ID,
    CONF_LIGHT,
    CONF_PULLUP,
    CONF_SCL,
    CONF_SDA,
    CONF_SENSOR,
    PLATFORM_ESP32,
    PLATFORM_ESP8266,
    PLATFORM_RP2040,
)
from esphome.core import CORE, coroutine_with_priority

CONF_COVER = "cover"
CONF_TARGET_ID = "target_id"
CONF_PREFIX = "prefix"
CONF_RX_BUFFER_SIZE = "receive_buffer"
CONF_SWITCH = "switch"
CONF_TEXT_SENSOR = "text_sensor"
CONF_TX_BUFFER_SIZE = "send_buffer"

CODEOWNERS = ["@tomdb-be"]

AUTO_LOAD = [
    "binary_sensor",
    "button",
    "cover",
    "light",
    "sensor",
    "switch",
    "text_sensor",
]

i2c_slave_device_ns = cg.esphome_ns.namespace("i2c_slave_device")
I2CIDFSlaveDevice = i2c_slave_device_ns.class_("I2CIDFSlaveDevice", cg.Component)

TargetType = i2c_slave_device_ns.enum("TargetType")
TARGET_TYPE = {
    "TYPE_NONE": TargetType.TYPE_NONE,
    "BINARY_SENSOR": TargetType.BINARY_SENSOR,
    "BUTTON": TargetType.BUTTON,
    "COVER": TargetType.COVER,
    "LIGHT": TargetType.LIGHT,
    "SENSOR": TargetType.SENSOR,
    "SWITCH": TargetType.SWITCH,
    "TEXT_SENSOR": TargetType.TEXT_SENSOR,
    "INVALID_TYPE": TargetType.INVALID_TYPE,
}


def _framework_declare_type(value):
    if CORE.using_esp_idf:
        return cv.declare_id(I2CIDFSlaveDevice)(value)
    raise NotImplementedError


CONFIG_I2C_SLAVE_BINARY_SENSOR_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(BinarySensor),
        cv.Optional(CONF_TARGET_ID, default=0x00): cv.hex_uint8_t,
    }
)

CONFIG_I2C_SLAVE_BUTTON_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(Button),
        cv.Optional(CONF_TARGET_ID, default=0x00): cv.hex_uint8_t,
    }
)

CONFIG_I2C_SLAVE_COVER_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(Cover),
        cv.Optional(CONF_TARGET_ID, default=0x00): cv.hex_uint8_t,
    }
)

CONFIG_I2C_SLAVE_LIGHT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(LightState),
        cv.Optional(CONF_TARGET_ID, default=0x00): cv.hex_uint8_t,
    }
)

CONFIG_I2C_SLAVE_SENSOR_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(Sensor),
        cv.Optional(CONF_TARGET_ID, default=0x00): cv.hex_uint8_t,
    }
)

CONFIG_I2C_SLAVE_SWITCH_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(Switch),
        cv.Optional(CONF_TARGET_ID, default=0x00): cv.hex_uint8_t,
    }
)

CONFIG_I2C_SLAVE_TEXT_SENSOR_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(TextSensor),
        cv.Optional(CONF_TARGET_ID, default=0x00): cv.hex_uint8_t,
    }
)

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
            cv.Optional(CONF_PREFIX, default=0xFF): cv.hex_uint32_t,
            cv.Optional(CONF_RX_BUFFER_SIZE, default=100): cv.All(
                cv.uint16_t, cv.Range(min=1)
            ),
            cv.Optional(CONF_TX_BUFFER_SIZE, default=100): cv.All(
                cv.uint16_t, cv.Range(min=1)
            ),
            cv.Optional(CONF_BINARY_SENSOR): cv.ensure_list(
                CONFIG_I2C_SLAVE_BINARY_SENSOR_SCHEMA
            ),
            cv.Optional(CONF_BUTTON): cv.ensure_list(CONFIG_I2C_SLAVE_BUTTON_SCHEMA),
            cv.Optional(CONF_COVER): cv.ensure_list(CONFIG_I2C_SLAVE_COVER_SCHEMA),
            cv.Optional(CONF_LIGHT): cv.ensure_list(CONFIG_I2C_SLAVE_LIGHT_SCHEMA),
            cv.Optional(CONF_SENSOR): cv.ensure_list(CONFIG_I2C_SLAVE_SENSOR_SCHEMA),
            cv.Optional(CONF_SWITCH): cv.ensure_list(CONFIG_I2C_SLAVE_SWITCH_SCHEMA),
            cv.Optional(CONF_TEXT_SENSOR): cv.ensure_list(
                CONFIG_I2C_SLAVE_TEXT_SENSOR_SCHEMA
            ),
        }
    ).extend(cv.polling_component_schema("2s")),
    cv.only_on([PLATFORM_ESP32, PLATFORM_ESP8266, PLATFORM_RP2040]),
    cv.only_with_esp_idf,
)


@coroutine_with_priority(1.0)
async def to_code(config):
    logging.warning(config)
    cg.add_global(i2c_slave_device_ns.using)

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_sda_pin(config[CONF_SDA]))
    cg.add(var.set_scl_pin(config[CONF_SCL]))
    cg.add(var.set_pullup(config[CONF_PULLUP]))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_prefix(config[CONF_PREFIX]))
    cg.add(var.set_rx_buffer_size(config[CONF_RX_BUFFER_SIZE]))
    cg.add(var.set_tx_buffer_size(config[CONF_TX_BUFFER_SIZE]))

    for target_name, target_type in TARGET_TYPE.items():
        for target_conf in config.get(target_name.lower(), ()):
            target_id = int(target_conf.get(CONF_TARGET_ID))
            if target_id > 0:
                target = await cg.get_variable(target_conf.get(CONF_ID))
                cg.add(var.set_component(target, target_type, target_id, str(target)))

    if CORE.using_arduino:
        cg.add_library("Wire", None)
