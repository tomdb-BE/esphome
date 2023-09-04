import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import cover, sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_GARAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CENTIMETER,
    ICON_ARROW_EXPAND_VERTICAL,    
)

AUTO_LOAD = ["sensor"]

ICON_GARAGE = "mdi:garage"

CONF_ULTRASONIC_COVER_ID = "ultrasonic_cover_id"

CONF_DOOR_ACTIVATE_PIN = "door_activate_pin"
CONF_DOOR_ACTIVE_PIN = "door_active_pin"

CONF_DOOR_SONAR = "door_sonar"
CONF_CAR_SONAR = "car_sonar"

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
CONF_SONAR_NAME = "name"

ultrasonic_cover_ns = cg.esphome_ns.namespace("ultrasonic_cover")
UltrasonicCover = ultrasonic_cover_ns.class_(
    "UltrasonicCover", cover.Cover, cg.Component
)
Sonar = ultrasonic_cover_ns.class_(
    "Sonar", sensor.Sensor, cg.PollingComponent
)

SONAR_SCHEMA = (sensor.sensor_schema(
        Sonar,
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
            cv.Optional(CONF_SONAR_NAME, default="0.1m"): cv.string
        }
    )
    .extend(cv.polling_component_schema("60s"))
)

CONFIG_SCHEMA = (
    cover.COVER_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(UltrasonicCover),
            cv.Required(CONF_DOOR_ACTIVATE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_DOOR_ACTIVE_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_DOOR_SONAR): SONAR_SCHEMA,     
            cv.Optional(CONF_CAR_SONAR): SONAR_SCHEMA,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def register_sonar(sonar_config):
    sonar = await sensor.new_sensor(sonar_config)
    await cg.register_component(sonar, sonar_config)

    trigger_pin = await cg.gpio_pin_expression(sonar_config[CONF_SONAR_TRIGGER_PIN])
    echo_pin = await cg.gpio_pin_expression(sonar_config[CONF_SONAR_ECHO_PIN])
    
    cg.add(sonar.set_trigger_pin(trigger_pin))    
    cg.add(sonar.set_echo_pin(echo_pin))    
    cg.add(sonar.set_min_distance(sonar_config[CONF_SONAR_MIN_DISTANCE]))
    cg.add(sonar.set_max_distance(sonar_config[CONF_SONAR_MAX_DISTANCE]))
    cg.add(sonar.set_timeout_distance(sonar_config[CONF_SONAR_TIMEOUT_DISTANCE]))
    cg.add(sonar.set_min_change(sonar_config[CONF_SONAR_MIN_CHANGE]))
    cg.add(sonar.set_update_interval(sonar_config[CONF_SONAR_UPDATE_INTERVAL]))
    cg.add(sonar.set_sleep_update_interval(sonar_config[CONF_SONAR_SLEEP_UPDATE_INTERVAL]))
    cg.add(sonar.set_sleep_timeout(sonar_config[CONF_SONAR_SLEEP_TIMEOUT]))
    cg.add(sonar.set_pulse_time_us(sonar_config[CONF_SONAR_PULSE_TIME]))
    cg.add(sonar.set_errors_ignored(sonar_config[CONF_SONAR_ERRORS_IGNORED]))
    
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cover.register_cover(var, config)

    activate_pin = await cg.gpio_pin_expression(config[CONF_DOOR_ACTIVATE_PIN])
    cg.add(var.set_activate_pin(activate_pin))    

    if CONF_DOOR_ACTIVE_PIN in config:
        active_pin = await cg.gpio_pin_expression(config[CONF_DOOR_ACTIVE_PIN])
        cg.add(var.set_active_pin(active_pin))  

    if CONF_DOOR_SONAR in config:
        config[CONF_DOOR_SONAR][CONF_SONAR_NAME] = "Door Sonar " + str(config[CONF_ID])
        await register_sonar(config[CONF_DOOR_SONAR])

    if CONF_CAR_SONAR in config:
        config[CONF_DOOR_SONAR][CONF_SONAR_NAME] =  "Car Sonar " + str(config[CONF_ID])
        await register_sonar(config[CONF_CAR_SONAR])   

  
    
