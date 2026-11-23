import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, output
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = []
CONF_DIRECTION_OUTPUT = "direction_output"
CONF_SCAN_START = "scan_start"
CONF_SCAN_END = "scan_end"
CONF_MAX_INVERTERS = "max_inverters"
CONF_POLL_INTERVAL = "poll_interval"

anenji_ns = cg.esphome_ns.namespace("anenji")
AnenjiManager = anenji_ns.class_("AnenjiManager", cg.PollingComponent, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(AnenjiManager),
    cv.Required(CONF_DIRECTION_OUTPUT): cv.use_id(output.BinaryOutput),
    cv.Optional(CONF_SCAN_START, default=1): cv.int_range(min=1, max=247),
    cv.Optional(CONF_SCAN_END, default=6): cv.int_range(min=1, max=247),
    cv.Optional(CONF_MAX_INVERTERS, default=6): cv.int_range(min=1, max=6),
    cv.Optional(CONF_POLL_INTERVAL, default="5s"): cv.positive_time_period_milliseconds,
}).extend(cv.polling_component_schema("5s")).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    direction = await cg.get_variable(config[CONF_DIRECTION_OUTPUT])
    cg.add(var.set_direction_output(direction))
    cg.add(var.set_scan_start(config[CONF_SCAN_START]))
    cg.add(var.set_scan_end(config[CONF_SCAN_END]))
    cg.add(var.set_max_inverters(config[CONF_MAX_INVERTERS]))
    cg.add(var.set_poll_ms(config[CONF_POLL_INTERVAL].total_milliseconds))
