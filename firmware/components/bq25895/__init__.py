import esphome.codegen as cg
from esphome.components import binary_sensor, i2c, sensor, text_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_BATTERY_CHARGING,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_PROBLEM,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_PERCENT,
    UNIT_VOLT,
)

DEPENDENCIES = ["i2c"]

CONF_INPUT_CURRENT_LIMIT = "input_current_limit"
CONF_CHARGE_CURRENT = "charge_current"
CONF_CHARGE_VOLTAGE = "charge_voltage"
CONF_PRECHARGE_CURRENT = "precharge_current"
CONF_TERMINATION_CURRENT = "termination_current"
CONF_CHARGE_ENABLED = "charge_enabled"
CONF_BATTERY_CAPACITY = "battery_capacity"
CONF_WARNING_PERCENT = "warning_percent"
CONF_CRITICAL_PERCENT = "critical_percent"
CONF_SHUTDOWN_PERCENT = "shutdown_percent"

CONF_BATTERY_VOLTAGE = "battery_voltage"
CONF_SYSTEM_VOLTAGE = "system_voltage"
CONF_INPUT_VOLTAGE = "input_voltage"
CONF_CHARGE_CURRENT_SENSOR = "charge_current_sensor"
CONF_INPUT_CURRENT_LIMIT_SENSOR = "input_current_limit_sensor"
CONF_BATTERY_PERCENT = "battery_percent"
CONF_TEMPERATURE_ADC = "temperature_adc"

CONF_CHARGING = "charging"
CONF_POWER_GOOD = "power_good"
CONF_BATTERY_FULL = "battery_full"
CONF_BOOST_ACTIVE = "boost_active"
CONF_CHARGER_FAULT = "charger_fault"
CONF_BATTERY_LOW = "battery_low"
CONF_BATTERY_CRITICAL = "battery_critical"
CONF_BATTERY_SHUTDOWN = "battery_shutdown"

CONF_CHARGE_STATUS = "charge_status"
CONF_FAULT_STATUS = "fault_status"

bq25895_ns = cg.esphome_ns.namespace("bq25895")
BQ25895Component = bq25895_ns.class_(
    "BQ25895Component", cg.PollingComponent, i2c.I2CDevice
)


def voltage_sensor_schema():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    )


def current_sensor_schema():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    )


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BQ25895Component),
            cv.Optional(CONF_INPUT_CURRENT_LIMIT, default=1000): cv.int_range(
                min=100, max=3250
            ),
            cv.Optional(CONF_CHARGE_CURRENT, default=640): cv.int_range(
                min=0, max=5056
            ),
            cv.Optional(CONF_CHARGE_VOLTAGE, default=4208): cv.int_range(
                min=3840, max=4608
            ),
            cv.Optional(CONF_PRECHARGE_CURRENT, default=128): cv.int_range(
                min=64, max=1024
            ),
            cv.Optional(CONF_TERMINATION_CURRENT, default=128): cv.int_range(
                min=64, max=1024
            ),
            cv.Optional(CONF_CHARGE_ENABLED, default=True): cv.boolean,
            cv.Optional(CONF_BATTERY_CAPACITY, default=1300): cv.positive_int,
            cv.Optional(CONF_WARNING_PERCENT, default=25): cv.int_range(
                min=0, max=100
            ),
            cv.Optional(CONF_CRITICAL_PERCENT, default=10): cv.int_range(
                min=0, max=100
            ),
            cv.Optional(CONF_SHUTDOWN_PERCENT, default=5): cv.int_range(
                min=0, max=100
            ),
            cv.Optional(CONF_BATTERY_VOLTAGE): voltage_sensor_schema(),
            cv.Optional(CONF_SYSTEM_VOLTAGE): voltage_sensor_schema(),
            cv.Optional(CONF_INPUT_VOLTAGE): voltage_sensor_schema(),
            cv.Optional(CONF_CHARGE_CURRENT_SENSOR): current_sensor_schema(),
            cv.Optional(
                CONF_INPUT_CURRENT_LIMIT_SENSOR
            ): current_sensor_schema(),
            cv.Optional(
                CONF_BATTERY_PERCENT
            ): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_BATTERY,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_TEMPERATURE_ADC
            ): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=1,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_CHARGING
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_BATTERY_CHARGING,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_POWER_GOOD
            ): binary_sensor.binary_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_BATTERY_FULL
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_BATTERY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_BOOST_ACTIVE
            ): binary_sensor.binary_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_CHARGER_FAULT
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_BATTERY_LOW
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_BATTERY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_BATTERY_CRITICAL
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_BATTERY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_BATTERY_SHUTDOWN
            ): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PROBLEM,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_CHARGE_STATUS
            ): text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(
                CONF_FAULT_STATUS
            ): text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(i2c.i2c_device_schema(0x6A))
)


async def _add_sensor(var, config, key, setter):
    if sensor_config := config.get(key):
        sens = await sensor.new_sensor(sensor_config)
        cg.add(getattr(var, setter)(sens))


async def _add_binary_sensor(var, config, key, setter):
    if binary_config := config.get(key):
        sens = await binary_sensor.new_binary_sensor(binary_config)
        cg.add(getattr(var, setter)(sens))


async def _add_text_sensor(var, config, key, setter):
    if text_config := config.get(key):
        sens = await text_sensor.new_text_sensor(text_config)
        cg.add(getattr(var, setter)(sens))


async def to_code(config):
    var = cg.new_Pvariable(
        config[CONF_ID],
        config[CONF_INPUT_CURRENT_LIMIT],
        config[CONF_CHARGE_CURRENT],
        config[CONF_CHARGE_VOLTAGE],
        config[CONF_PRECHARGE_CURRENT],
        config[CONF_TERMINATION_CURRENT],
        config[CONF_CHARGE_ENABLED],
        config[CONF_BATTERY_CAPACITY],
        config[CONF_WARNING_PERCENT],
        config[CONF_CRITICAL_PERCENT],
        config[CONF_SHUTDOWN_PERCENT],
    )
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    await _add_sensor(var, config, CONF_BATTERY_VOLTAGE, "set_battery_voltage_sensor")
    await _add_sensor(var, config, CONF_SYSTEM_VOLTAGE, "set_system_voltage_sensor")
    await _add_sensor(var, config, CONF_INPUT_VOLTAGE, "set_input_voltage_sensor")
    await _add_sensor(
        var, config, CONF_CHARGE_CURRENT_SENSOR, "set_charge_current_sensor"
    )
    await _add_sensor(
        var,
        config,
        CONF_INPUT_CURRENT_LIMIT_SENSOR,
        "set_input_current_limit_sensor",
    )
    await _add_sensor(var, config, CONF_BATTERY_PERCENT, "set_battery_percent_sensor")
    await _add_sensor(var, config, CONF_TEMPERATURE_ADC, "set_temperature_adc_sensor")

    await _add_binary_sensor(var, config, CONF_CHARGING, "set_charging_sensor")
    await _add_binary_sensor(var, config, CONF_POWER_GOOD, "set_power_good_sensor")
    await _add_binary_sensor(var, config, CONF_BATTERY_FULL, "set_battery_full_sensor")
    await _add_binary_sensor(var, config, CONF_BOOST_ACTIVE, "set_boost_active_sensor")
    await _add_binary_sensor(var, config, CONF_CHARGER_FAULT, "set_charger_fault_sensor")
    await _add_binary_sensor(var, config, CONF_BATTERY_LOW, "set_battery_low_sensor")
    await _add_binary_sensor(
        var, config, CONF_BATTERY_CRITICAL, "set_battery_critical_sensor"
    )
    await _add_binary_sensor(
        var, config, CONF_BATTERY_SHUTDOWN, "set_battery_shutdown_sensor"
    )

    await _add_text_sensor(var, config, CONF_CHARGE_STATUS, "set_charge_status_sensor")
    await _add_text_sensor(var, config, CONF_FAULT_STATUS, "set_fault_status_sensor")