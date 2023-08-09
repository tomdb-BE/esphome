import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID


ultrasonic_cover_ns = cg.esphome_ns.namespace("ultrasonic_cover")
UltrasonicCover = ultrasonic_cover_ns.class_(
    "UltrasonicCover", cg.Component
)
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(UltrasonicCover),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)  