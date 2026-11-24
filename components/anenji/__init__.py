import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, output, sensor, text_sensor, switch, number
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "text_sensor", "switch", "number"]
CONF_DIRECTION_OUTPUT = "direction_output"
CONF_SCAN_START = "scan_start"
CONF_SCAN_END = "scan_end"
CONF_MAX_INVERTERS = "max_inverters"
CONF_POLL_INTERVAL = "poll_interval"

anenji_ns = cg.esphome_ns.namespace("anenji")
AnenjiManager = anenji_ns.class_("AnenjiManager", cg.PollingComponent, uart.UARTDevice)

EcoSwitch = anenji_ns.class_("EcoSwitch", switch.Switch)

SettingNumber = anenji_ns.class_("SettingNumber", number.Number)

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
        serial = await text_sensor.new_text_sensor(text_sensor.text_sensor_schema()({"name": f"{prefix} Serial", CONF_ID: f"{config[CONF_ID]}_slot_{slot}_serial"}))
        cg.add(var.set_serial_sensor(slot, serial))
        addr = await sensor.new_sensor(sensor.sensor_schema(accuracy_decimals=0)({"name": f"{prefix} Modbus Address", CONF_ID: f"{config[CONF_ID]}_slot_{slot}_address"}))
        cg.add(var.set_address_sensor(slot, addr))
        for metric, (name, unit, decimals, device_class) in enumerate([["Battery Voltage","V",1,"voltage"],["Battery Current","A",1,"current"],["Battery SOC","%",0,"battery"],["AC Output Voltage","V",1,"voltage"],["AC Generation Power","W",0,"power"],["Grid Voltage","V",1,"voltage"],["Grid Current","A",1,"current"],["Grid Power","W",0,"power"],["DC Bus Voltage","V",1,"voltage"],["Temperature 1","°C",1,"temperature"],["Temperature 2","°C",1,"temperature"],["Temperature 3","°C",1,"temperature"],["AC Energy Today","kWh",1,"energy"],["Battery Discharge Today","kWh",1,"energy"],["PV Energy Total","kWh",1,"energy"],["Load Energy Total","kWh",1,"energy"],["AC Energy Total","kWh",1,"energy"],["Battery Charging Power","W",0,"power"],["AC Charging Current","A",1,"current"],["PV Charging Current","A",1,"current"],["AC Output Power","W",0,"power"],["AC Output Apparent Power","VA",0,"apparent_power"],["AC Output Load","%",0,None],["Grid Frequency","Hz",2,"frequency"],["Battery Charge Today","kWh",1,"energy"],["PV Energy Today","kWh",1,"energy"],["Load Energy Today","kWh",1,"energy"],["Battery Charge Total","kWh",1,"energy"],["Battery Discharge Total","kWh",1,"energy"],["Grid Charging Energy Today","kWh",1,"energy"],["Load Energy From Grid Today","kWh",1,"energy"],["PV Voltage","V",1,"voltage"],["PV Current","A",1,"current"],["PV Power","W",0,"power"],["AC Output Current","A",1,"current"],["AC Output Frequency","Hz",2,"frequency"]]):
            s = await sensor.new_sensor(sensor.sensor_schema(unit_of_measurement=unit, accuracy_decimals=decimals, **({"device_class": device_class} if device_class else {}), state_class="total_increasing" if unit == "kWh" else "measurement")({"name": f"{prefix} {name}", CONF_ID: f"{config[CONF_ID]}_slot_{slot}_metric_{metric}"}))
            cg.add(var.set_metric(slot, metric, s))

        eco = await switch.new_switch(switch.switch_schema(EcoSwitch)({"name": f"{prefix} Eco Mode", CONF_ID: f"{config[CONF_ID]}_slot_{slot}_eco"}))
        cg.add(eco.configure(var, slot))
        cg.add(var.set_eco(slot, eco))
        for index, (reg, name, minimum, maximum, scale, unit) in enumerate([[41007,"Maximum Charging Current",0,80,10,"A"],[41008,"Solar Charging Current",0,80,10,"A"],[41009,"Grid Charging Current",0,60,10,"A"],[41013,"Charge Full Current",2,15,10,"A"],[41115,"SOC Alarm",0,100,1,"%"],[41116,"SOC Discharge Stop",0,100,1,"%"],[41117,"SOC Full",0,100,1,"%"],[41118,"Inverter to Grid SOC",0,100,1,"%"],[41119,"Grid to Inverter SOC",10,100,1,"%"]]):
            n = await number.new_number(number.number_schema(SettingNumber, unit_of_measurement=unit)({"name": f"{prefix} {name}", CONF_ID: f"{config[CONF_ID]}_slot_{slot}_number_{reg}"}), min_value=minimum, max_value=maximum, step=1)
            cg.add(n.configure(var, slot, index))
            cg.add(var.set_setting_number(slot, index, n))
        for index, reg in enumerate([40000, 40001, 41000, 41001, 41002, 41003, 41004, 41005, 41006, 41007, 41008, 41009, 41011, 41012, 41013, 41014, 41018, 41022, 41023, 41025, 41027, 41031, 41032, 41065, 41078, 41079, 41080, 41081, 41091, 41100, 41101, 41102, 41104, 41105, 41106, 41107, 41108, 41109, 41112, 41113, 41114, 41115, 41116, 41117, 41118, 41119, 41120, 41121, 41122, 41123, 41124, 41127, 41128, 41129, 41130, 41131, 41135, 41138, 41103, 41110, 41111]):
            s = await sensor.new_sensor(sensor.sensor_schema(accuracy_decimals=0)({"name": f"{prefix} Setting {reg}", CONF_ID: f"{config[CONF_ID]}_slot_{slot}_setting_{reg}"}))
            cg.add(var.set_metric(slot, 36 + index, s))

    for index, (name, unit, decimals, device_class, state_class) in enumerate([
        ("Discovered Inverters", "", 0, None, "measurement"),
        ("Registered Inverters", "", 0, None, "measurement"),
        ("System Battery Current Sum", "A", 1, "current", "measurement"),
        ("System AC Generation Power", "W", 0, "power", "measurement"),
        ("System Grid Power", "W", 0, "power", "measurement"),
        ("System AC Energy Today", "kWh", 1, "energy", "total_increasing"),
        ("System Battery Discharge Today", "kWh", 1, "energy", "total_increasing"),
    ]):
        options = {"accuracy_decimals": decimals, "state_class": state_class}
        if unit:
            options["unit_of_measurement"] = unit
        if device_class:
            options["device_class"] = device_class
        s = await sensor.new_sensor(sensor.sensor_schema(**options)({
            "name": name, CONF_ID: f"{config[CONF_ID]}_system_{index}"
        }))
        cg.add(var.set_system_sensor(index, s))
