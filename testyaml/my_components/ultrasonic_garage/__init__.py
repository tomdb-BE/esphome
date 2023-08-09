import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import cover, sensor
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    DEVICE_CLASS_GARAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CENTIMETER,
    ICON_ARROW_EXPAND_VERTICAL,    
)

AUTO_LOAD = ["cover", "sensor"]

ICON_GARAGE = "mdi:garage"

CONF_GARAGE_GATE = "gate"
CONF_GARAGE_SONAR_GATE = "gate_sonar"
CONF_GARAGE_SONAR_CAR = "car_sonar"
CONF_GARAGE_UPDATE_INTERVAL = "update_interval"

CONF_GATE_ID = "gate_id"
CONF_GATE_ACTIVATE_PIN = "activate_pin"
CONF_GATE_ACTIVE_PIN = "active_pin"
CONF_GATE_MIN_POSITION_DELTA = "min_position_delta"
CONF_GATE_TRIGGER_TIME = "trigger_time"
CONF_GATE_OPERATION_TIMEOUT = "operation_timeout"


CONF_SONAR_ID = "sonar_id"
CONF_SONAR_TRIGGER_PIN = "trigger_pin"
CONF_SONAR_ECHO_PIN = "echo_pin"
CONF_SONAR_MIN_DISTANCE = "min"
CONF_SONAR_MAX_DISTANCE = "max"
CONF_SONAR_TIMEOUT_DISTANCE = "timeout"
CONF_SONAR_MIN_CHANGE = "min_change"
CONF_SONAR_SLEEP_UPDATE_INTERVAL = "sleep_update_interval"
CONF_SONAR_SLEEP_TIMEOUT = "sleep"
CONF_SONAR_PULSE_TIME = "pulse_time"
CONF_SONAR_ERRORS_IGNORED = "errors_ignored"


ultrasonic_garage_ns = cg.esphome_ns.namespace("ultrasonic_garage")

UltrasonicGarage = ultrasonic_garage_ns.class_(
    "UltrasonicGarage", cg.PollingComponent
)
UltrasonicGarageGate = ultrasonic_garage_ns.class_(
    "UltrasonicGarageGate", cover.Cover, cg.Component
)
UltrasonicGarageSonar = ultrasonic_garage_ns.class_(
    "UltrasonicGarageSonar", sensor.Sensor, cg.Component
)
GATE_SCHEMA = (
    cover.COVER_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(UltrasonicGarageGate),
            cv.Required(CONF_GATE_ACTIVATE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_NAME, default="Ultrasonic Garage Gate"): cv.string,
            cv.Optional(CONF_GATE_ACTIVE_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_GATE_MIN_POSITION_DELTA, default=0.05): cv.percentage,
            cv.Optional(CONF_GATE_TRIGGER_TIME, default="400ms"):  cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=cv.TimePeriod(milliseconds=20), max=cv.TimePeriod(milliseconds=2000)),
            ),
            cv.Optional(CONF_GATE_OPERATION_TIMEOUT, default="120s"):  cv.All(
                cv.positive_time_period_seconds,
                cv.Range(min=cv.TimePeriod(seconds=2), max=cv.TimePeriod(seconds=600)),
            ),                      
        }
    )
)

SONAR_SCHEMA = (sensor.sensor_schema(
        UltrasonicGarageSonar,
        unit_of_measurement=UNIT_CENTIMETER,
        icon=ICON_ARROW_EXPAND_VERTICAL,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(): cv.declare_id(UltrasonicGarageSonar),  
            cv.Required(CONF_SONAR_TRIGGER_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_SONAR_ECHO_PIN): pins.internal_gpio_input_pin_schema,
            cv.Optional(CONF_NAME, default="UNSET"): cv.string,
            cv.Optional(CONF_SONAR_MIN_DISTANCE, default="0.1m"): cv.distance,
            cv.Optional(CONF_SONAR_MAX_DISTANCE, default="2m"): cv.distance,    
            cv.Optional(CONF_SONAR_TIMEOUT_DISTANCE, default="3m"): cv.distance,
            cv.Optional(CONF_SONAR_MIN_CHANGE, default="0.02m"): cv.distance,
            cv.Optional(CONF_SONAR_SLEEP_UPDATE_INTERVAL, default="60s"):  cv.All(
                cv.positive_time_period_seconds,
                cv.Range(min=cv.TimePeriod(seconds=60)),
            ),
            cv.Optional(CONF_SONAR_SLEEP_TIMEOUT, default="2min"): cv.All(
                cv.positive_time_period_seconds,
                cv.Range(min=cv.TimePeriod(seconds=120)),
            ),
            cv.Optional(CONF_SONAR_PULSE_TIME, default="10us"): cv.positive_time_period_microseconds,
            cv.Optional(CONF_SONAR_ERRORS_IGNORED, default=0): cv.positive_int,
        }
    )    
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(UltrasonicGarage),        
            cv.Required(CONF_GARAGE_GATE): GATE_SCHEMA,
            cv.Optional(CONF_GARAGE_UPDATE_INTERVAL, default="200ms"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=cv.TimePeriod(milliseconds=16), max=cv.TimePeriod(milliseconds=4000)),
            ),            
            cv.Optional(CONF_NAME, default="Ultrasonic Garage"): cv.string,
            cv.Optional(CONF_GARAGE_SONAR_GATE): SONAR_SCHEMA,
            cv.Optional(CONF_GARAGE_SONAR_CAR): SONAR_SCHEMA,                             
        }
    )
).extend(cv.polling_component_schema("10s"))

async def add_gate(gate_config):
    gate_ptr = cg.new_Pvariable(gate_config[CONF_ID])
    await cover.register_cover(gate_ptr, gate_config)

    activate_pin = await cg.gpio_pin_expression(gate_config[CONF_GATE_ACTIVATE_PIN])
    cg.add(gate_ptr.set_activate_pin(activate_pin))    

    if CONF_GATE_ACTIVE_PIN in gate_config:
        active_pin = await cg.gpio_pin_expression(gate_config[CONF_GATE_ACTIVE_PIN])
        cg.add(gate_ptr.set_active_pin(active_pin))

    cg.add(gate_ptr.set_min_position_delta(gate_config[CONF_GATE_MIN_POSITION_DELTA]))
    cg.add(gate_ptr.set_trigger_time(gate_config[CONF_GATE_TRIGGER_TIME]))
    cg.add(gate_ptr.set_operation_timeout(gate_config[CONF_GATE_OPERATION_TIMEOUT]))    

    return gate_ptr


async def add_sonar(sonar_config, is_car_sonar = False):
    sonar_ptr = cg.new_Pvariable(sonar_config[CONF_ID])    
    await sensor.register_sensor(sonar_ptr, sonar_config)    

    trigger_pin = await cg.gpio_pin_expression(sonar_config[CONF_SONAR_TRIGGER_PIN])
    echo_pin = await cg.gpio_pin_expression(sonar_config[CONF_SONAR_ECHO_PIN])
    
    cg.add(sonar_ptr.set_sonar_type(is_car_sonar))
    cg.add(sonar_ptr.set_trigger_pin(trigger_pin))    
    cg.add(sonar_ptr.set_echo_pin(echo_pin))    
    cg.add(sonar_ptr.set_min_distance(sonar_config[CONF_SONAR_MIN_DISTANCE]))
    cg.add(sonar_ptr.set_max_distance(sonar_config[CONF_SONAR_MAX_DISTANCE]))
    cg.add(sonar_ptr.set_timeout_distance(sonar_config[CONF_SONAR_TIMEOUT_DISTANCE]))
    cg.add(sonar_ptr.set_min_change(sonar_config[CONF_SONAR_MIN_CHANGE]))
    cg.add(sonar_ptr.set_sleep_update_interval(sonar_config[CONF_SONAR_SLEEP_UPDATE_INTERVAL]))
    cg.add(sonar_ptr.set_sleep_timeout(sonar_config[CONF_SONAR_SLEEP_TIMEOUT]))
    cg.add(sonar_ptr.set_pulse_time_us(sonar_config[CONF_SONAR_PULSE_TIME]))
    cg.add(sonar_ptr.set_errors_ignored(sonar_config[CONF_SONAR_ERRORS_IGNORED]))

    return sonar_ptr
    
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    gate = await add_gate(config[CONF_GARAGE_GATE])
    cg.add(var.set_gate(gate))
    if CONF_GARAGE_SONAR_GATE in config:
        sonar_gate_config = config[CONF_GARAGE_SONAR_GATE]
        if sonar_gate_config[CONF_NAME] == "UNSET":
            sonar_gate_config[CONF_NAME] = config[CONF_NAME] + " Gate Sonar"
        sonar_gate = await add_sonar(sonar_gate_config)
        cg.add(var.set_sonar_gate(sonar_gate))
    if CONF_GARAGE_SONAR_CAR in config:
        sonar_car_config = config[CONF_GARAGE_SONAR_CAR]
        if sonar_car_config[CONF_NAME] == "UNSET":
            sonar_car_config[CONF_NAME] = config[CONF_NAME] + " Car Sonar"        
        sonar_car = await add_sonar(sonar_car_config, True)   
        cg.add(var.set_sonar_car(sonar_car))
    cg.add(var.set_update_interval(config[CONF_GARAGE_UPDATE_INTERVAL]))

  
    
