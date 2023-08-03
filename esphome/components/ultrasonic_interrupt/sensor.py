import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import sensor
from esphome.const import (
    CONF_ECHO_PIN,
    CONF_TRIGGER_PIN,
    CONF_TIMEOUT,
    STATE_CLASS_MEASUREMENT,
    UNIT_CENTIMETER,
    ICON_ARROW_EXPAND_VERTICAL,
)

CONF_PULSE_TIME = "pulse_time"

ultrasonic_interrupt_ns = cg.esphome_ns.namespace("ultrasonic_interrupt")
UltrasonicInterruptSensorComponent = ultrasonic_interrupt_ns.class_(
    "UltrasonicInterruptSensorComponent", sensor.Sensor, cg.PollingComponent
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        UltrasonicInterruptSensorComponent,
        unit_of_measurement=UNIT_CENTIMETER,
        icon=ICON_ARROW_EXPAND_VERTICAL,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.Required(CONF_TRIGGER_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_ECHO_PIN): pins.internal_gpio_input_pin_schema,      
            cv.Optional(CONF_TIMEOUT, default="2m"): cv.distance,
            cv.Optional(
                CONF_PULSE_TIME, default="10us"
            ): cv.positive_time_period_microseconds,
        }
    )
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    trigger = await cg.gpio_pin_expression(config[CONF_TRIGGER_PIN])
    cg.add(var.set_trigger_pin(trigger))
    echo = await cg.gpio_pin_expression(config[CONF_ECHO_PIN])
    cg.add(var.set_echo_pin(echo))

    cg.add(var.set_timeout(config[CONF_TIMEOUT]))
    cg.add(var.set_pulse_time_us(config[CONF_PULSE_TIME]))
