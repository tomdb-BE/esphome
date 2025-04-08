from esphome import automation
import esphome.codegen as cg
from esphome.components.i2c import pin_with_input_and_output_support
import esphome.config_validation as cv
from esphome.const import (
    CONF_ADDRESS,
    CONF_ID,
    CONF_PULLUP,
    CONF_SCL,
    CONF_SDA,
    CONF_TRIGGER_ID,
    CONF_VALUE,
)
from esphome.core import CORE, coroutine_with_priority

CODEOWNERS = ["@tomdb-be"]

CONF_ON_RECEIVE = "on_receive"
CONF_ON_RECEIVE_VALUE = "on_receive_value"
CONF_RX_BUFFER_SIZE = "receive_buffer"
CONF_TX_BUFFER_SIZE = "request_buffer"

MULTI_CONF = True

i2c_slave_device_ns = cg.esphome_ns.namespace("i2c_slave_device")
I2CSlaveDevice = i2c_slave_device_ns.class_("I2CSlaveDevice", cg.Component)
I2CSlaveDeviceOnReceiveTrigger = i2c_slave_device_ns.class_(
    "I2CSlaveDeviceOnReceiveTrigger", automation.Trigger.template(cg.std_string)
)
I2CSlaveDeviceWriteAction = i2c_slave_device_ns.class_(
    "I2CSlaveDeviceWriteAction", automation.Action
)
Filter = i2c_slave_device_ns.class_("Filter")
I2CSlaveDeviceHasValueCondition = i2c_slave_device_ns.class_(
    "I2CSlaveDeviceHasValueCondition", Filter
)


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
            cv.Optional(CONF_RX_BUFFER_SIZE, default=256): cv.All(
                cv.uint16_t, cv.Range(min=101)
            ),
            cv.Optional(CONF_TX_BUFFER_SIZE, default=256): cv.All(
                cv.uint16_t, cv.Range(min=101)
            ),
            cv.Optional(CONF_ON_RECEIVE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        I2CSlaveDeviceOnReceiveTrigger
                    ),
                    cv.Optional(CONF_VALUE, default="____NONE____"): cv.string_strict,
                }
            ),
        }
    ),
)


@coroutine_with_priority(1.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_sda_pin(config[CONF_SDA]))
    cg.add(var.set_scl_pin(config[CONF_SCL]))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_rx_buffer_size(config[CONF_RX_BUFFER_SIZE]))
    cg.add(var.set_tx_buffer_size(config[CONF_TX_BUFFER_SIZE]))

    for conf in config.get(CONF_ON_RECEIVE, []):
        trigger = cg.new_Pvariable(conf.get(CONF_TRIGGER_ID), var)
        trigger_value = conf.get(CONF_VALUE)
        if trigger_value == "____NONE____":
            cg.add(trigger.set_trigger_always())
        else:
            cg.add(trigger.set_trigger_value(trigger_value))
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)

    if CORE.using_arduino:
        cg.add_library("Wire", None)
    else:
        cg.add(var.set_pullup(config[CONF_PULLUP]))


OPERATION_BASE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(I2CSlaveDevice),
    }
)


@automation.register_action(
    "i2c_slave_device.write",
    I2CSlaveDeviceWriteAction,
    OPERATION_BASE_SCHEMA.extend(
        {
            cv.Required(CONF_VALUE): cv.templatable(cv.string_strict),
        }
    ),
)
async def i2c_slave_device_write_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    template_ = await cg.templatable(config[CONF_VALUE], args, cg.std_string)
    cg.add(var.write(template_))
    return var


I2C_SLAVE_DEVICE_HAS_VALUE_CONDITION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(I2CSlaveDevice),
        cv.Required(CONF_VALUE): cv.templatable(cv.string_strict),
    }
)


@automation.register_condition(
    "i2c_slave_device.has_value",
    I2CSlaveDeviceHasValueCondition,
    I2C_SLAVE_DEVICE_HAS_VALUE_CONDITION_SCHEMA,
)
async def i2c_slave_device_has_value_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(condition_id, template_arg, parent)
    cg.add(var.set_value(config.get(CONF_VALUE)))
    return var
