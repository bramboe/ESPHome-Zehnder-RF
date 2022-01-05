import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan
from esphome.const import CONF_ID

from esphome.components.nrf905 import nRF905Component


DEPENDENCIES = ["nrf905"]

zehnder_ns = cg.esphome_ns.namespace("zehnder")
ZehnderRF = zehnder_ns.class_("ZehnderRF", fan.FanState)

CONF_NRF905 = "nrf905"

CONFIG_SCHEMA = fan.FAN_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ZehnderRF),
        cv.Required(CONF_NRF905): cv.use_id(nRF905Component),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await fan.register_fan(var, config)

    nrf905 = await cg.get_variable(config[CONF_NRF905])
    cg.add(var.set_rf(nrf905))
