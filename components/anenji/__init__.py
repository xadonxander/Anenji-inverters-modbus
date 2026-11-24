import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, output, sensor, text_sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor"]
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

    for slot in range(config[CONF_MAX_INVERTERS]):
        prefix = f"Inverter {slot + 1}"
        serial = await text_sensor.new_text_sensor(text_sensor.text_sensor_schema()({"name": f"{prefix} Serial"}))
        cg.add(var.set_serial_sensor(slot, serial))
        addr = await sensor.new_sensor(sensor.sensor_schema(accuracy_decimals=0)({"name": f"{prefix} Modbus Address"}))
        cg.add(var.set_address_sensor(slot, addr))
        for metric, (name, unit, decimals, device_class) in enumerate([["Battery Voltage","V",1,"voltage"],["Battery Current Raw","A",1,"current"],["Battery SOC","%",0,"battery"],["AC Output Voltage","V",1,"voltage"],["AC Generation Power","W",0,"power"],["Grid Voltage","V",1,"voltage"],["Grid Current","A",1,"current"],["Grid Power","W",0,"power"],["DC Bus Voltage","V",1,"voltage"],["Temperature 1","°C",1,"temperature"],["Temperature 2","°C",1,"temperature"],["Temperature 3","°C",1,"temperature"],["AC Energy Today","kWh",1,"energy"],["Battery Discharge Today","kWh",1,"energy"],["PV Energy Total Low Word","kWh",1,"energy"],["Used Energy Total Low Word","kWh",1,"energy"],["AC Energy Total Low Word","kWh",1,"energy"]]):
            s = await sensor.new_sensor(sensor.sensor_schema(unit_of_measurement=unit, accuracy_decimals=decimals, device_class=device_class)({"name": f"{prefix} {name}"}))
            cg.add(var.set_metric(slot, metric, s))
