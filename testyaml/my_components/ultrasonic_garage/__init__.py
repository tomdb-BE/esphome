import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.core import ID
from esphome.automation import validate_automation, build_automation, register_action, Trigger
from esphome.components.light.types import AddressableLightEffect, LightControlAction
from esphome.components.light.effects import register_addressable_effect
from esphome.components.light.automation import LIGHT_TURN_ON_ACTION_SCHEMA, light_control_to_code
from esphome import pins
from esphome.components import cover, sensor, binary_sensor
from esphome.const import (    
    CONF_ID,
    CONF_SENSOR_ID,
    CONF_NAME,
    CONF_PIN,
    CONF_DEVICE_CLASS,
    CONF_TRIGGER_ID,
    DEVICE_CLASS_GATE,
    DEVICE_CLASS_GARAGE,
    DEVICE_CLASS_GARAGE_DOOR,    
    DEVICE_CLASS_DISTANCE,
    DEVICE_CLASS_MOTION,
    STATE_CLASS_MEASUREMENT,
    UNIT_CENTIMETER,
    ICON_ARROW_EXPAND_VERTICAL,
)

AUTO_LOAD = ["cover", "sensor", "binary_sensor", "light"]

CONF_EFFECT_MIRRORED = "mirrored"
CONF_EFFECT_REVERSED = "reversed"

ICON_GARAGE = "mdi:garage"
CONF_LIGHT_CONTROLLER_ID = "light_control_id"
CONF_LIGHT_CONTROLLER_ACTIONS = "actions"
CONF_LIGHT_CONTROLLER_OPENING = "opening"
CONF_LIGHT_CONTROLLER_CLOSING = "closing"
CONF_LIGHT_CONTROLLER_DISTANCE_GATE = "distance_gate"
CONF_LIGHT_CONTROLLER_DISTANCE_CAR = "distance_car"
CONF_LIGHT_CONTROLLER_MOTION = "motion"
CONF_LIGHT_CONTROLLER_IDLE = "idle"

CONF_GATE_ID = "gate_id"
CONF_GATE_ACTIVATE_PIN = "activate_pin"
CONF_GATE_ACTIVE_PIN = "active_pin"
CONF_GATE_MIN_POSITION_DELTA = "min_position_delta"
CONF_GATE_TRIGGER_TIME = "trigger_time"
CONF_GATE_OPERATION_TIMEOUT = "operation_timeout"

CONF_SONAR_ID = "sonar_id"
CONF_SONAR_TRIGGER_PIN = "trigger_pin"
CONF_SONAR_ECHO_PIN = "echo_pin"
CONF_SONAR_MIN_DISTANCE = "min_distance"
CONF_SONAR_MAX_DISTANCE = "max_distance"
CONF_SONAR_TIMEOUT_DISTANCE = "timeout"
CONF_SONAR_MIN_CHANGE = "min_change"
CONF_SONAR_SLEEP_UPDATE_INTERVAL = "sleep_update_interval"
CONF_SONAR_SLEEP_TIMEOUT = "sleep"
CONF_SONAR_PULSE_TIME = "pulse_time"
CONF_SONAR_MAX_ERRORS = "max_errors"

CONF_MOTION_INVERTED = "inverted"
CONF_MOTION_MIN_ON = "min_on"
CONF_MOTION_MIN_OFF = "min_off"

CONF_GARAGE_GATE = "gate"
CONF_GARAGE_UPDATE_INTERVAL = "update_interval"
CONF_GARAGE_SONAR_GATE = "gate_sonar"
CONF_GARAGE_SENSOR_GATE = "gate_sensor"
CONF_GARAGE_SONAR_CAR = "car_sonar"
CONF_GARAGE_MOTION = "motion_sensor"
CONF_GARAGE_LIGHT_CONTROLLER = "light_control"

ultrasonic_garage_ns = cg.esphome_ns.namespace("ultrasonic_garage")

UltrasonicGarage = ultrasonic_garage_ns.class_(
    "UltrasonicGarage", cg.PollingComponent
)
UltrasonicGarageGate = ultrasonic_garage_ns.class_(
    "UltrasonicGarageGate", cover.Cover
)
UltrasonicGarageSonar = ultrasonic_garage_ns.class_(
    "UltrasonicGarageSonar", cg.Component, sensor.Sensor
)
UltrasonicGarageMotion = ultrasonic_garage_ns.class_(
    "UltrasonicGarageMotion", cg.Component, binary_sensor.BinarySensor
)
UltrasonicGarageLightController = ultrasonic_garage_ns.class_(
    "UltrasonicGarageLightController"
)
UltrasonicGarageActionType = ultrasonic_garage_ns.enum(
    "UltrasonicGarageActionType"
)
UltrasonicGarageLightControllerTrigger = ultrasonic_garage_ns.class_(
    "UltrasonicGarageLightControllerTrigger", Trigger
)

ACTION_TYPE = {
  "OPENING": UltrasonicGarageActionType.OPENING,
  "CLOSING": UltrasonicGarageActionType.CLOSING,
  "DISTANCE_GATE": UltrasonicGarageActionType.DISTANCE_GATE,
  "DISTANCE_CAR": UltrasonicGarageActionType.DISTANCE_CAR,
  "MOTION": UltrasonicGarageActionType.MOTION,
  "IDLE": UltrasonicGarageActionType.IDLE,
}

ScanFastLightEffect = ultrasonic_garage_ns.class_(
    "ScanFastLightEffect", cg.Component, AddressableLightEffect
)
@register_addressable_effect(
    "scan_fast",
    ScanFastLightEffect,
    "Scan Fast",
    {
        cv.Optional(CONF_EFFECT_MIRRORED, default=False) : cv.boolean,
        cv.Optional(CONF_EFFECT_REVERSED, default=False) : cv.boolean,
    },
)
async def scanfast_light_effect_to_code(config, effect_id):
    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])
    cg.add(effect.set_mirrored(config[CONF_EFFECT_MIRRORED]))
    cg.add(effect.set_reversed(config[CONF_EFFECT_REVERSED]))    
    return effect

FillFastLightEffect = ultrasonic_garage_ns.class_(
    "FillFastLightEffect", AddressableLightEffect
)
@register_addressable_effect(
    "fill_fast",
    FillFastLightEffect,
    "Fill Fast",
    {
        cv.Optional(CONF_EFFECT_MIRRORED, default=False) : cv.boolean,
        cv.Optional(CONF_EFFECT_REVERSED, default=False) : cv.boolean,
    },
)
async def fill_fast_light_effect_to_code(config, effect_id):
    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])
    cg.add(effect.set_mirrored(config[CONF_EFFECT_MIRRORED]))
    cg.add(effect.set_reversed(config[CONF_EFFECT_REVERSED]))
    return effect

GateDistanceLightEffect = ultrasonic_garage_ns.class_(
    "GateDistanceLightEffect", AddressableLightEffect
)
@register_addressable_effect(
    "gate_distance",
    GateDistanceLightEffect,
    "Gate Distance",
    {
        cv.Optional(CONF_SENSOR_ID) : cv.use_id(UltrasonicGarageSonar),
        cv.Optional(CONF_EFFECT_MIRRORED, default=False) : cv.boolean,        
    },
)
async def gate_distance_light_effect_to_code(config, effect_id):
    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])
    if CONF_SENSOR_ID in config:
        sonar_sensor = await cg.get_variable(config[CONF_SENSOR_ID])
        cg.add(effect.set_sonar_sensor(sonar_sensor))
    cg.add(effect.set_mirrored(config[CONF_EFFECT_MIRRORED]))
    return effect

LIGHT_CONTROLLER_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_LIGHT_CONTROLLER_ID): cv.declare_id(UltrasonicGarageLightController),          
        cv.Optional(CONF_LIGHT_CONTROLLER_OPENING): validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(UltrasonicGarageLightControllerTrigger),
            }
        ),
        cv.Optional(CONF_LIGHT_CONTROLLER_CLOSING): validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(UltrasonicGarageLightControllerTrigger),
            }
        ),
        cv.Optional(CONF_LIGHT_CONTROLLER_DISTANCE_CAR): validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(UltrasonicGarageLightControllerTrigger),
            }
        ),
        cv.Optional(CONF_LIGHT_CONTROLLER_DISTANCE_GATE): validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(UltrasonicGarageLightControllerTrigger),
            }
        ),
        cv.Optional(CONF_LIGHT_CONTROLLER_MOTION): validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(UltrasonicGarageLightControllerTrigger),
            }
        ),
        cv.Optional(CONF_LIGHT_CONTROLLER_IDLE): validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(UltrasonicGarageLightControllerTrigger),
            }
        ),                                        
    }
)

@register_action("set_light", LightControlAction, LIGHT_TURN_ON_ACTION_SCHEMA)
async def set_light_to_code(config, action_id, template_arg, args):
    return await light_control_to_code(config, action_id, template_arg, args)

GATE_SCHEMA = (cover.COVER_SCHEMA.extend(
        {   
            cv.GenerateID(): cv.declare_id(UltrasonicGarageGate),
            cv.Required(CONF_GATE_ACTIVATE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_NAME, default="Gate"): cv.string,
            cv.Optional(CONF_GATE_MIN_POSITION_DELTA, default=0.05): cv.percentage,
            cv.Optional(CONF_DEVICE_CLASS, default=DEVICE_CLASS_GARAGE_DOOR): cv.one_of(DEVICE_CLASS_GARAGE_DOOR, DEVICE_CLASS_GATE),
            cv.Optional(CONF_GATE_TRIGGER_TIME, default="400ms"): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=cv.TimePeriod(milliseconds=20), max=cv.TimePeriod(milliseconds=2000)),
            ),
            cv.Optional(CONF_GATE_OPERATION_TIMEOUT, default="120s"): cv.All(
                cv.positive_time_period_seconds,
                cv.Range(min=cv.TimePeriod(seconds=2), max=cv.TimePeriod(seconds=600)),
            ),                      
        }
    ).extend(cv.COMPONENT_SCHEMA)
)

SONAR_SCHEMA = (sensor.sensor_schema(
        UltrasonicGarageSonar,
        unit_of_measurement=UNIT_CENTIMETER,
        icon=ICON_ARROW_EXPAND_VERTICAL,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
        device_class=DEVICE_CLASS_DISTANCE
    )
    .extend(
        {
            cv.Required(CONF_SONAR_TRIGGER_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_SONAR_ECHO_PIN): pins.internal_gpio_input_pin_schema,                           
            cv.Optional(CONF_SONAR_MIN_DISTANCE, default="10cm"): cv.All(cv.distance, cv.float_range(max=650)),
            cv.Optional(CONF_SONAR_MAX_DISTANCE, default="200cm"): cv.All(cv.distance, cv.float_range(max=650)), 
            cv.Optional(CONF_SONAR_TIMEOUT_DISTANCE, default="300cm"): cv.All(cv.distance, cv.float_range(max=650)),
            cv.Optional(CONF_SONAR_MIN_CHANGE, default="2cm"): cv.All(cv.distance, cv.float_range(max=650)),
            cv.Optional(CONF_SONAR_SLEEP_UPDATE_INTERVAL, default="60s"):  cv.All(cv.positive_time_period_seconds, cv.Range(min=cv.TimePeriod(seconds=60))),            
            cv.Optional(CONF_SONAR_SLEEP_TIMEOUT, default="2min"): cv.All(cv.positive_time_period_seconds, cv.Range(min=cv.TimePeriod(seconds=120))),
            cv.Optional(CONF_SONAR_PULSE_TIME, default="10us"): cv.All(cv.positive_time_period_microseconds, cv.Range(max=cv.TimePeriod(microseconds=1000))),
            cv.Optional(CONF_SONAR_MAX_ERRORS, default=0): cv.positive_int,
        }
    )    
)
SONAR_GATE_SCHEMA = (SONAR_SCHEMA.extend(
        {
            cv.Optional(CONF_NAME, default="Sonar Sensor Gate"): cv.string,
        }
    )  
)
SONAR_CAR_SCHEMA = (SONAR_SCHEMA.extend(
        {
            cv.Optional(CONF_NAME, default="Sonar Sensor Car"): cv.string,
        }
    )  
)

MOTION_SCHEMA = (binary_sensor.binary_sensor_schema(
        UltrasonicGarageMotion,
        device_class=DEVICE_CLASS_MOTION
    )
    .extend(
        {
            cv.Required(CONF_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_NAME, default="UNSET"): cv.string,           
            cv.Optional(CONF_MOTION_INVERTED, default=False): cv.boolean,
            cv.Optional(CONF_MOTION_MIN_ON, default="0ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_MOTION_MIN_OFF, default="0ms"): cv.positive_time_period_milliseconds,
        }
    )    
)
SENSOR_GATE_SCHEMA = (MOTION_SCHEMA.extend(
        {
            cv.Optional(CONF_NAME, default="Gate Sensor"): cv.string,
        }
    )  
)
GARAGE_MOTION_SCHEMA = (MOTION_SCHEMA.extend(
        {
            cv.Optional(CONF_NAME, default="Motion Sensor"): cv.string,
        }
    )  
)

CONFIG_SCHEMA = (cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(UltrasonicGarage),        
            cv.Required(CONF_GARAGE_GATE): GATE_SCHEMA,
            cv.Optional(CONF_DEVICE_CLASS, default=DEVICE_CLASS_GARAGE): cv.one_of(DEVICE_CLASS_GARAGE, DEVICE_CLASS_GATE),           
            cv.Optional(CONF_NAME, default="Ultrasonic Garage"): cv.string,
            cv.Optional(CONF_GARAGE_SONAR_GATE): SONAR_GATE_SCHEMA,
            cv.Optional(CONF_GARAGE_SONAR_CAR): SONAR_CAR_SCHEMA,
            cv.Optional(CONF_GARAGE_SENSOR_GATE): SENSOR_GATE_SCHEMA,
            cv.Optional(CONF_GARAGE_MOTION) : GARAGE_MOTION_SCHEMA,
            cv.Optional(CONF_GARAGE_LIGHT_CONTROLLER) : LIGHT_CONTROLLER_SCHEMA,
        }
    ).extend(cv.polling_component_schema("200ms"))
)

async def add_gate(gate_config):      
    gate_ptr = cg.new_Pvariable(gate_config[CONF_ID])
    await cover.register_cover(gate_ptr, gate_config)

    activate_pin = await cg.gpio_pin_expression(gate_config[CONF_GATE_ACTIVATE_PIN])
    cg.add(gate_ptr.set_activate_pin(activate_pin))    

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
    cg.add(sonar_ptr.set_min_distance(sonar_config[CONF_SONAR_MIN_DISTANCE] * 100))
    cg.add(sonar_ptr.set_max_distance(sonar_config[CONF_SONAR_MAX_DISTANCE] * 100))
    cg.add(sonar_ptr.set_timeout_distance(sonar_config[CONF_SONAR_TIMEOUT_DISTANCE] * 100))
    cg.add(sonar_ptr.set_min_change(sonar_config[CONF_SONAR_MIN_CHANGE] * 100))
    cg.add(sonar_ptr.set_sleep_update_interval(sonar_config[CONF_SONAR_SLEEP_UPDATE_INTERVAL]))
    cg.add(sonar_ptr.set_sleep_timeout(sonar_config[CONF_SONAR_SLEEP_TIMEOUT]))
    cg.add(sonar_ptr.set_pulse_time_us(sonar_config[CONF_SONAR_PULSE_TIME]))
    cg.add(sonar_ptr.set_max_errors(sonar_config[CONF_SONAR_MAX_ERRORS]))

    return sonar_ptr

async def add_motion_sensor(motion_config):    
    motion_ptr = cg.new_Pvariable(motion_config[CONF_ID])    
    await binary_sensor.register_binary_sensor(motion_ptr, motion_config)    

    motion_pin = await cg.gpio_pin_expression(motion_config[CONF_PIN])

    cg.add(motion_ptr.set_motion_pin(motion_pin)) 
    cg.add(motion_ptr.set_inverted(motion_config[CONF_MOTION_INVERTED]))
    cg.add(motion_ptr.set_min_on(motion_config[CONF_MOTION_MIN_ON]))
    cg.add(motion_ptr.set_min_off(motion_config[CONF_MOTION_MIN_OFF]))

    return motion_ptr

async def add_light_controller(light_controller_config):
    light_controller = cg.new_Pvariable(light_controller_config[CONF_LIGHT_CONTROLLER_ID])
    for action_type_config in light_controller_config:
        if action_type_config == "light_control_id":
            continue
        action_type = str(action_type_config)        
        for automation_config in light_controller_config.get(action_type_config, []):                
            trigger = cg.new_Pvariable(automation_config[CONF_TRIGGER_ID])
            await build_automation(trigger, [], automation_config)                
            cg.add(light_controller.add_light_action(trigger, ACTION_TYPE[action_type.upper()]))
    return light_controller

async def to_code(config):    
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    gate_config = config[CONF_GARAGE_GATE]
    gate = await add_gate(gate_config)
    cg.add(var.set_gate(gate))
    if CONF_GARAGE_SONAR_GATE in config:
        sonar_gate_config = config[CONF_GARAGE_SONAR_GATE]
        sonar_gate = await add_sonar(sonar_gate_config)
        cg.add(var.set_sonar_gate(sonar_gate))        
    if CONF_GARAGE_SONAR_CAR in config:
        sonar_car_config = config[CONF_GARAGE_SONAR_CAR]
        sonar_car = await add_sonar(sonar_car_config, True)   
        cg.add(var.set_sonar_car(sonar_car))
    if CONF_GARAGE_SENSOR_GATE in config:
        gate_sensor_config = config[CONF_GARAGE_SENSOR_GATE]
        gate_sensor = await add_motion_sensor(gate_sensor_config)             
        cg.add(var.set_gate_sensor(gate_sensor))   
    if CONF_GARAGE_MOTION in config:
        motion_sensor_config = config[CONF_GARAGE_MOTION]
        motion_sensor = await add_motion_sensor(motion_sensor_config)                     
        cg.add(var.set_motion_sensor(motion_sensor))
    if CONF_GARAGE_LIGHT_CONTROLLER in config:
        light_controller_config = config[CONF_GARAGE_LIGHT_CONTROLLER]        
        light_controller = await add_light_controller(light_controller_config)
        cg.add(var.set_light_controller(light_controller))    
