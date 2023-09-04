import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.core import ID
from esphome.automation import validate_automation, build_automation, register_action, Trigger, maybe_simple_id
from esphome.components.light.types import AddressableLightEffect, LightControlAction
from esphome.components.light.effects import EFFECTS_REGISTRY, ADDRESSABLE_EFFECTS, register_addressable_effect, validate_effects
from esphome.components.light.automation import LIGHT_CONTROL_ACTION_SCHEMA, LIGHT_TURN_ON_ACTION_SCHEMA, light_control_to_code
from esphome import pins
from esphome.components import cover, sensor, binary_sensor
from esphome.const import (    
    CONF_ID,
    CONF_STATE,
    CONF_NAME,
    CONF_PIN,
    CONF_THEN,
    CONF_EFFECTS,
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

EXTRA_EFFECTS_BASE = ["scan_fast", "fill_fast", "distance_gate", "distance_car"]

EXTRA_EFFECTS = []
for base_effect in EXTRA_EFFECTS_BASE:
    EXTRA_EFFECTS.append(base_effect)
    EXTRA_EFFECTS.append(base_effect + "_mirrored")
    EXTRA_EFFECTS.append(base_effect + "_reversed")
    EXTRA_EFFECTS.append(base_effect + "_mirrored_reversed")

ScanFastLightEffect = ultrasonic_garage_ns.class_(
    "ScanFastLightEffect", AddressableLightEffect
)
FillFastLightEffect = ultrasonic_garage_ns.class_(
    "FillFastLightEffect", AddressableLightEffect
)
DistanceGateLightEffect = ultrasonic_garage_ns.class_(
    "DistanceGateLightEffect", AddressableLightEffect
)
DistanceCarLightEffect = ultrasonic_garage_ns.class_(
    "DistanceCarLightEffect", AddressableLightEffect
)
@register_addressable_effect("scan_fast", ScanFastLightEffect, "Scan Fast", {})
@register_addressable_effect("scan_fast_mirrored", ScanFastLightEffect, "Scan Fast Mirrored", {})
@register_addressable_effect("scan_fast_reversed", ScanFastLightEffect, "Scan Fast Reversed", {})
@register_addressable_effect("scan_fast_mirrored_reversed", ScanFastLightEffect, "Scan Fast Mirrored Reversed", {})
@register_addressable_effect("fill_fast", FillFastLightEffect, "Fill Fast", {})
@register_addressable_effect("fill_fast_mirrored", FillFastLightEffect, "Fill Fast Mirrored", {})
@register_addressable_effect("fill_fast_reversed", FillFastLightEffect, "Fill Fast Reversed", {})
@register_addressable_effect("fill_fast_mirrored_reversed", FillFastLightEffect, "Fill Fast Mirrored Reversed", {})
@register_addressable_effect("distance_gate", DistanceGateLightEffect, "Distance gate", {})
@register_addressable_effect("distance_gate_mirrored", DistanceGateLightEffect, "Distance gate Mirrored", {})
@register_addressable_effect("distance_gate_reversed", DistanceGateLightEffect, "Distance gate Reversed", {})
@register_addressable_effect("distance_gate_mirrored_reversed", DistanceGateLightEffect, "Distance gate Mirrored Reversed", {})
@register_addressable_effect("distance_car", DistanceCarLightEffect, "Distance car", {})
@register_addressable_effect("distance_car_mirrored", DistanceCarLightEffect, "Distance car Mirrored", {})
@register_addressable_effect("distance_car_reversed", DistanceCarLightEffect, "Distance car Reversed", {})
@register_addressable_effect("distance_car_mirrored_reversed", DistanceCarLightEffect, "Distance car Mirrored Reversed", {})
async def ultrasonic_garage_light_effects_to_code(config, effect_id):
    effect = cg.new_Pvariable(effect_id, config[CONF_NAME])    
    if "MIRRORED" in config[CONF_NAME].upper():
        cg.add(effect.set_mirrored(True))
    if "REVERSED" in config[CONF_NAME].upper():
        cg.add(effect.set_reversed(True))    
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
        cv.Optional(CONF_EFFECTS, default=EXTRA_EFFECTS): validate_effects(ADDRESSABLE_EFFECTS),
    }
)

SET_LIGHT_ACTION_SCHEMA = maybe_simple_id(LIGHT_CONTROL_ACTION_SCHEMA.extend(
        {
            cv.Optional(CONF_STATE, default=True): True,
            cv.Optional(CONF_EFFECTS, default=EXTRA_EFFECTS): validate_effects(ADDRESSABLE_EFFECTS),
        }
    )
)

@register_action("set_light", LightControlAction, SET_LIGHT_ACTION_SCHEMA)
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
            cv.Optional(CONF_SONAR_SLEEP_UPDATE_INTERVAL, default="60s"):  cv.All(cv.positive_time_period_microseconds, cv.Range(min=cv.TimePeriod(seconds=60))),            
            cv.Optional(CONF_SONAR_SLEEP_TIMEOUT, default="2min"): cv.All(cv.positive_time_period_microseconds, cv.Range(min=cv.TimePeriod(seconds=120))),
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
            cv.Optional(CONF_MOTION_MIN_ON, default="1s"): cv.positive_time_period_microseconds,
            cv.Optional(CONF_MOTION_MIN_OFF, default="5s"): cv.positive_time_period_microseconds,
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

async def add_light_controller(light_controller_config, gate, sonar_car):
    light_controller = cg.new_Pvariable(light_controller_config[CONF_LIGHT_CONTROLLER_ID])
    light_states = []
    for action_type_config in light_controller_config:
        if action_type_config == "light_control_id" or action_type_config == "effects":
            continue
        action_type = str(action_type_config)        
        for automation_config in light_controller_config.get(action_type_config, []):                            
            for light_state_config in automation_config[CONF_THEN]:                
                if "set_light" in light_state_config:                    
                    light_state = await cg.get_variable(light_state_config["set_light"][CONF_ID])
                    if str(light_state) not in str(light_states):
                        light_effects = await cg.build_registry_list(EFFECTS_REGISTRY, light_state_config["set_light"].get(CONF_EFFECTS, []))
                        for light_effect in light_effects:
                            if gate is not None and "ultrasonic_garage_distancegatelighteffect" in str(light_effect):
                                cg.add(light_effect.set_gate(gate))
                            if sonar_car is not None and "ultrasonic_garage_distancecarlighteffect" in str(light_effect):
                                cg.add(light_effect.set_sonar_sensor(sonar_car))                                
                        cg.add(light_state.add_effects(light_effects)) 
                        light_states.append(light_state)
            light_trigger = cg.new_Pvariable(automation_config[CONF_TRIGGER_ID])
            await build_automation(light_trigger, [], automation_config)
            if light_states:
                cg.add(light_controller.add_light_states(light_states))                
            cg.add(light_controller.add_light_trigger(light_trigger, ACTION_TYPE[action_type.upper()]))
    return light_controller

async def to_code(config):    
    sonar_car = None
    var = cg.new_Pvariable(config[CONF_ID])    
    gate_config = config[CONF_GARAGE_GATE]
    gate = await add_gate(gate_config)
    cg.add(var.set_gate(gate))
    if CONF_GARAGE_SONAR_GATE in config:
        sonar_gate_config = config[CONF_GARAGE_SONAR_GATE]
        sonar_gate = await add_sonar(sonar_gate_config, False)
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
        light_controller = await add_light_controller(light_controller_config, gate, sonar_car)
        cg.add(var.set_light_controller(light_controller))
    await cg.register_component(var, config)
