#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

#include <cstdint>
#include <string>

namespace esphome::bq25895 {

class BQ25895Component final : public PollingComponent, public i2c::I2CDevice {
 public:
  BQ25895Component(uint16_t input_current_limit_ma, uint16_t charge_current_ma,
                   uint16_t charge_voltage_mv, uint16_t precharge_current_ma,
                   uint16_t termination_current_ma, bool charge_enabled,
                   uint16_t battery_capacity_mah, uint8_t warning_percent,
                   uint8_t critical_percent, uint8_t shutdown_percent);

  void setup() override;
  void dump_config() override;
  void update() override;

  void set_battery_voltage_sensor(sensor::Sensor *sensor) {
    battery_voltage_sensor_ = sensor;
  }
  void set_system_voltage_sensor(sensor::Sensor *sensor) {
    system_voltage_sensor_ = sensor;
  }
  void set_input_voltage_sensor(sensor::Sensor *sensor) {
    input_voltage_sensor_ = sensor;
  }
  void set_charge_current_sensor(sensor::Sensor *sensor) {
    charge_current_sensor_ = sensor;
  }
  void set_input_current_limit_sensor(sensor::Sensor *sensor) {
    input_current_limit_sensor_ = sensor;
  }
  void set_battery_percent_sensor(sensor::Sensor *sensor) {
    battery_percent_sensor_ = sensor;
  }
  void set_temperature_adc_sensor(sensor::Sensor *sensor) {
    temperature_adc_sensor_ = sensor;
  }

  void set_charging_sensor(binary_sensor::BinarySensor *sensor) {
    charging_sensor_ = sensor;
  }
  void set_power_good_sensor(binary_sensor::BinarySensor *sensor) {
    power_good_sensor_ = sensor;
  }
  void set_battery_full_sensor(binary_sensor::BinarySensor *sensor) {
    battery_full_sensor_ = sensor;
  }
  void set_boost_active_sensor(binary_sensor::BinarySensor *sensor) {
    boost_active_sensor_ = sensor;
  }
  void set_charger_fault_sensor(binary_sensor::BinarySensor *sensor) {
    charger_fault_sensor_ = sensor;
  }
  void set_battery_low_sensor(binary_sensor::BinarySensor *sensor) {
    battery_low_sensor_ = sensor;
  }
  void set_battery_critical_sensor(binary_sensor::BinarySensor *sensor) {
    battery_critical_sensor_ = sensor;
  }
  void set_battery_shutdown_sensor(binary_sensor::BinarySensor *sensor) {
    battery_shutdown_sensor_ = sensor;
  }

  void set_charge_status_sensor(text_sensor::TextSensor *sensor) {
    charge_status_sensor_ = sensor;
  }
  void set_fault_status_sensor(text_sensor::TextSensor *sensor) {
    fault_status_sensor_ = sensor;
  }

 protected:
  bool configure_();
  bool read_register_(uint8_t reg, uint8_t *value);
  bool write_register_(uint8_t reg, uint8_t value);
  bool update_register_(uint8_t reg, uint8_t mask, uint8_t value);
  bool read_status_block_(uint8_t *data);

  static uint8_t encode_input_current_limit_(uint16_t milliamps);
  static uint8_t encode_charge_current_(uint16_t milliamps);
  static uint8_t encode_voltage_limit_(uint16_t millivolts);
  static uint8_t encode_linear_current_(uint16_t milliamps);
  static float estimate_battery_percent_(float voltage);
  static const char *charge_status_text_(uint8_t charge_status);
  static std::string fault_status_text_(uint8_t fault_register);

  uint16_t input_current_limit_ma_;
  uint16_t charge_current_ma_;
  uint16_t charge_voltage_mv_;
  uint16_t precharge_current_ma_;
  uint16_t termination_current_ma_;
  bool charge_enabled_;
  uint16_t battery_capacity_mah_;
  uint8_t warning_percent_;
  uint8_t critical_percent_;
  uint8_t shutdown_percent_;

  sensor::Sensor *battery_voltage_sensor_{nullptr};
  sensor::Sensor *system_voltage_sensor_{nullptr};
  sensor::Sensor *input_voltage_sensor_{nullptr};
  sensor::Sensor *charge_current_sensor_{nullptr};
  sensor::Sensor *input_current_limit_sensor_{nullptr};
  sensor::Sensor *battery_percent_sensor_{nullptr};
  sensor::Sensor *temperature_adc_sensor_{nullptr};

  binary_sensor::BinarySensor *charging_sensor_{nullptr};
  binary_sensor::BinarySensor *power_good_sensor_{nullptr};
  binary_sensor::BinarySensor *battery_full_sensor_{nullptr};
  binary_sensor::BinarySensor *boost_active_sensor_{nullptr};
  binary_sensor::BinarySensor *charger_fault_sensor_{nullptr};
  binary_sensor::BinarySensor *battery_low_sensor_{nullptr};
  binary_sensor::BinarySensor *battery_critical_sensor_{nullptr};
  binary_sensor::BinarySensor *battery_shutdown_sensor_{nullptr};

  text_sensor::TextSensor *charge_status_sensor_{nullptr};
  text_sensor::TextSensor *fault_status_sensor_{nullptr};
};

}  // namespace esphome::bq25895