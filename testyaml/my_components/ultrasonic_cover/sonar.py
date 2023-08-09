import esphome.config_validation as cv
import esphome.codegen as cg
from esphome import pins
from esphome.components import sensor
from esphome.const import (
    STATE_CLASS_MEASUREMENT,
    UNIT_CENTIMETER,
    ICON_ARROW_EXPAND_VERTICAL,
)

CONF_SONAR_ID = "sonar_id"
CONF_SONAR_TRIGGER_PIN = "trigger_pin"
CONF_SONAR_ECHO_PIN = "echo_pin"
CONF_SONAR_MIN_DISTANCE = "min"
CONF_SONAR_MAX_DISTANCE = "max"
CONF_SONAR_TIMEOUT_DISTANCE = "timeout"
CONF_SONAR_MIN_CHANGE = "min_change"
CONF_SONAR_UPDATE_INTERVAL = "update_interval"
CONF_SONAR_SLEEP_UPDATE_INTERVAL = "sleep_update_interval"
CONF_SONAR_SLEEP_TIMEOUT = "sleep"
CONF_SONAR_PULSE_TIME = "pulse_time"
CONF_SONAR_ERRORS_IGNORED = "errors_ignored"

ultrasonic_interrupt_ns = cg.esphome_ns.namespace("ultrasonic_cover")
UltrasonicInterruptSensorComponent = ultrasonic_interrupt_ns.class_(
    "Sonar", sensor.Sensor, cg.PollingComponent
)


sonar_schema = (sensor.sensor_schema(
        UltrasonicInterruptSensorComponent,
        unit_of_measurement=UNIT_CENTIMETER,
        icon=ICON_ARROW_EXPAND_VERTICAL,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.Required(CONF_SONAR_TRIGGER_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_SONAR_ECHO_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_SONAR_MIN_DISTANCE, default="0.1m"): cv.distance,
            cv.Optional(CONF_SONAR_MAX_DISTANCE, default="2m"): cv.distance,    
            cv.Optional(CONF_SONAR_TIMEOUT_DISTANCE, default="3m"): cv.distance,
            cv.Optional(CONF_SONAR_MIN_CHANGE, default="0.02m"): cv.distance,
            cv.Optional(CONF_SONAR_UPDATE_INTERVAL, default="1min"): cv.All(
                cv.positive_time_period_seconds,
                cv.Range(min=cv.TimePeriod(milliseconds=1)),
            ),
            cv.Optional(CONF_SONAR_SLEEP_UPDATE_INTERVAL, default="1min"):  cv.All(
                cv.positive_time_period_seconds,
                cv.Range(min=cv.TimePeriod(milliseconds=1)),
            ),
            cv.Optional(CONF_SONAR_SLEEP_TIMEOUT, default="2min"): cv.All(
                cv.positive_time_period_seconds,
                cv.Range(min=cv.TimePeriod(seconds=1)),
            ),
            cv.Optional(CONF_SONAR_PULSE_TIME, default="10us"): cv.positive_time_period_microseconds,
            cv.Optional(CONF_SONAR_ERRORS_IGNORED, default=0): cv.positive_int,
        }
    )
)

async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    trigger_pin = await cg.gpio_pin_expression(config[CONF_SONAR_TRIGGER_PIN])
    echo_pin = await cg.gpio_pin_expression(config[CONF_SONAR_ECHO_PIN])
    
    cg.add(var.set_trigger_pin(trigger_pin))    
    cg.add(var.set_echo_pin(echo_pin))    
    cg.add(var.set_min_distance(config[CONF_SONAR_MIN_DISTANCE]))
    cg.add(var.set_max_distance(config[CONF_SONAR_MAX_DISTANCE]))
    cg.add(var.set_timeout_distance(config[CONF_SONAR_TIMEOUT_DISTANCE]))
    cg.add(var.set_min_change(config[CONF_SONAR_MIN_CHANGE]))
    cg.add(var.set_update_interval(config[CONF_SONAR_UPDATE_INTERVAL]))
    cg.add(var.set_sleep_update_interval(config[CONF_SONAR_SLEEP_UPDATE_INTERVAL]))
    cg.add(var.set_sleep_timeout(config[CONF_SONAR_SLEEP_TIMEOUT]))
    cg.add(var.set_pulse_time_us(config[CONF_SONAR_PULSE_TIME]))
    cg.add(var.set_errors_ignored(config[CONF_SONAR_ERRORS_IGNORED]))
    