import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import light
from esphome.const import (
    CONF_CHIPSET,
    CONF_MAX_REFRESH_RATE,
    CONF_NUM_LEDS,
    CONF_OUTPUT_ID,
    CONF_PIN,
    CONF_RGB_ORDER,
)

CODEOWNERS = ["@tomdb-be"]
DEPENDENCIES = ["esp32"]

esp32_spi_led_strip_ns = cg.esphome_ns.namespace("esp32_spi_led_strip")
ESP32SPILEDStripLightOutput = esp32_spi_led_strip_ns.class_(
    "ESP32SPILEDStripLightOutput", light.AddressableLight
)

RGBOrder = esp32_spi_led_strip_ns.enum("RGBOrder")

DMA_CHANNELS = {    
    "1": 1,
    "2": 2,
    "auto": 3,
    "disabled": 0
}

RGB_ORDERS = {
    "RGB": RGBOrder.ORDER_RGB,
    "RBG": RGBOrder.ORDER_RBG,
    "GRB": RGBOrder.ORDER_GRB,
    "GBR": RGBOrder.ORDER_GBR,
    "BGR": RGBOrder.ORDER_BGR,
    "BRG": RGBOrder.ORDER_BRG,
}

CHIPSETS = {
    "WS2812": 3200000,
    "SK6812": 3200000,
    "APA106": 3200000,
    "SM16703": 3200000,
}

CONF_DMA_CHANNEL = "dma_channel"
CONF_IS_RGBW = "is_rgbw"

CONFIG_SCHEMA = cv.All(
    light.ADDRESSABLE_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(ESP32SPILEDStripLightOutput),
            cv.Required(CONF_PIN): pins.internal_gpio_output_pin_number,
            cv.Required(CONF_NUM_LEDS): cv.positive_not_null_int,
            cv.Required(CONF_RGB_ORDER): cv.enum(RGB_ORDERS, upper=True),
            cv.Optional(CONF_MAX_REFRESH_RATE): cv.positive_time_period_microseconds,
            cv.Optional(CONF_DMA_CHANNEL, default="auto"): cv.one_of(*DMA_CHANNELS),
            cv.Optional(CONF_CHIPSET, default="WS2812"): cv.one_of(*CHIPSETS, upper=True),
            cv.Optional(CONF_IS_RGBW, default=False): cv.boolean,
        }
    )    
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)
    await cg.register_component(var, config)

    cg.add(var.set_num_leds(config[CONF_NUM_LEDS]))
    cg.add(var.set_pin(config[CONF_PIN]))

    if CONF_MAX_REFRESH_RATE in config:
        cg.add(var.set_max_refresh_rate(config[CONF_MAX_REFRESH_RATE]))
    
    cg.add(var.set_dma_channel(DMA_CHANNELS[config[CONF_DMA_CHANNEL]]))
    cg.add(var.set_clock_speed(CHIPSETS[config[CONF_CHIPSET]]))
    cg.add(var.set_rgb_order(config[CONF_RGB_ORDER]))
    cg.add(var.set_is_rgbw(config[CONF_IS_RGBW]))
