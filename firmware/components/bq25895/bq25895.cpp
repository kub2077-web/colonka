#include "bq25895.h"

#include "esphome/core/log.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace esphome::bq25895 {

static const char *const TAG = "bq25895";

namespace {

constexpr uint8_t REG_INPUT_SOURCE = 0x00;
constexpr uint8_t REG_POWER_ON = 0x01;
constexpr uint8_t REG_ADC_CONTROL = 0x02;
constexpr uint8_t REG_POWER_CONTROL = 0x03;
constexpr uint8_t REG_CHARGE_CURRENT = 0x04;
constexpr uint8_t REG_PRECHARGE_TERMINATION = 0x05;
constexpr uint8_t REG_CHARGE_VOLTAGE = 0x06;
constexpr uint8_t REG_CHARGE_TIMER = 0x07;
constexpr uint8_t REG_MISC_OPERATION = 0x09;
constexpr uint8_t REG_STATUS = 0x0B;
constexpr uint8_t REG_FAULT = 0x0C;
constexpr uint8_t REG_BATTERY_VOLTAGE = 0x0E;
constexpr uint8_t REG_SYSTEM_VOLTAGE = 0x0F;
constexpr uint8_t REG_TEMPERATURE_ADC = 0x10;
constexpr uint8_t REG_INPUT_VOLTAGE = 0x11;
constexpr uint8_t REG_CHARGE_CURRENT_ADC = 0x12;
constexpr uint8_t REG_INPUT_CURRENT_ADC = 0x13;
constexpr uint8_t REG_PART_NUMBER = 0x14;

constexpr uint8_t INPUT_CURRENT_MASK = 0x3F;
constexpr uint8_t CHARGE_CURRENT_MASK = 0x7F;
constexpr uint8_t CHARGE_VOLTAGE_MASK = 0xFC;
constexpr uint8_t PRECHARGE_MASK = 0xF0;
constexpr uint8_t TERMINATION_MASK = 0x0F;
constexpr uint8_t CHARGE_ENABLE_BIT = 0x10;

}  // namespace

BQ25895Component::BQ25895Component(
    uint16_t input_current_limit_ma, uint16_t charge_current_ma,
    uint16_t charge_voltage_mv, uint16_t precharge_current_ma,
    uint16_t termination_current_ma, bool charge_enabled,
    uint16_t battery_capacity_mah, uint8_t warning_percent,
    uint8_t critical_percent, uint8_t shutdown_percent)
    : PollingComponent(10000),
      input_current_limit_ma_(input_current_limit_ma),
      charge_current_ma_(charge_current_ma),
      charge_voltage_mv_(charge_voltage_mv),
      precharge_current_ma_(precharge_current_ma),
      termination_current_ma_(termination_current_ma),
      charge_enabled_(charge_enabled),
      battery_capacity_mah_(battery_capacity_mah),
      warning_percent_(warning_percent),
      critical_percent_(critical_percent),
      shutdown_percent_(shutdown_percent) {}

void BQ25895Component::setup() {
  if (!this->configure_()) {
    this->mark_failed();
    return;
  }

  uint8_t part_number = 0;
  if (this->read_register_(REG_PART_NUMBER, &part_number)) {
    const uint8_t part = (part_number >> 3) & 0x07;
    if (part != 0x07) {
      ESP_LOGW(TAG, "Unexpected BQ25895 part number code: 0x%02X", part);
    }
  }
}

void BQ25895Component::dump_config() {
  ESP_LOGCONFIG(TAG, "BQ25895 battery controller:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  ESP_LOGCONFIG(TAG, "  Input current limit: %u mA", input_current_limit_ma_);
  ESP_LOGCONFIG(TAG, "  Charge current: %u mA", charge_current_ma_);
  ESP_LOGCONFIG(TAG, "  Charge voltage: %u mV", charge_voltage_mv_);
  ESP_LOGCONFIG(TAG, "  Precharge current: %u mA", precharge_current_ma_);
  ESP_LOGCONFIG(TAG, "  Termination current: %u mA", termination_current_ma_);
  ESP_LOGCONFIG(TAG, "  Charge enabled: %s", charge_enabled_ ? "yes" : "no");
  ESP_LOGCONFIG(TAG, "  Battery capacity: %u mAh", battery_capacity_mah_);
  ESP_LOGCONFIG(TAG, "  Warning threshold: %u%%", warning_percent_);
  ESP_LOGCONFIG(TAG, "  Critical threshold: %u%%", critical_percent_);
  ESP_LOGCONFIG(TAG, "  Shutdown threshold: %u%%", shutdown_percent_);

  LOG_SENSOR("  ", "Battery voltage", battery_voltage_sensor_);
  LOG_SENSOR("  ", "System voltage", system_voltage_sensor_);
  LOG_SENSOR("  ", "Input voltage", input_voltage_sensor_);
  LOG_SENSOR("  ", "Charge current", charge_current_sensor_);
  LOG_SENSOR("  ", "Input current limit", input_current_limit_sensor_);
  LOG_SENSOR("  ", "Battery percentage", battery_percent_sensor_);
  LOG_SENSOR("  ", "NTC ADC level", temperature_adc_sensor_);
  LOG_BINARY_SENSOR("  ", "Charging", charging_sensor_);
  LOG_BINARY_SENSOR("  ", "Power good", power_good_sensor_);
  LOG_BINARY_SENSOR("  ", "Battery full", battery_full_sensor_);
  LOG_BINARY_SENSOR("  ", "Boost active", boost_active_sensor_);
  LOG_BINARY_SENSOR("  ", "Charger fault", charger_fault_sensor_);
  LOG_BINARY_SENSOR("  ", "Battery low", battery_low_sensor_);
  LOG_BINARY_SENSOR("  ", "Battery critical", battery_critical_sensor_);
  LOG_BINARY_SENSOR("  ", "Battery shutdown", battery_shutdown_sensor_);
  LOG_TEXT_SENSOR("  ", "Charge status", charge_status_sensor_);
  LOG_TEXT_SENSOR("  ", "Fault status", fault_status_sensor_);
}

void BQ25895Component::update() {
  uint8_t registers[10] = {};
  if (!this->read_status_block_(registers)) {
    this->status_set_warning();
    return;
  }

  const uint8_t status = registers[REG_STATUS - REG_STATUS];
  const uint8_t fault = registers[REG_FAULT - REG_STATUS];
  const uint8_t charge_status = (status >> 3) & 0x03;
  const uint8_t input_status = (status >> 5) & 0x07;

  const float battery_voltage =
      2.304f + static_cast<float>(registers[REG_BATTERY_VOLTAGE - REG_STATUS] & 0x7F) * 0.020f;
  const float system_voltage =
      2.304f + static_cast<float>(registers[REG_SYSTEM_VOLTAGE - REG_STATUS] & 0x7F) * 0.020f;
  const float temperature_adc =
      static_cast<float>(registers[REG_TEMPERATURE_ADC - REG_STATUS] & 0x7F) * 0.00465f + 0.21f;
  const float input_voltage =
      2.600f + static_cast<float>(registers[REG_INPUT_VOLTAGE - REG_STATUS] & 0x7F) * 0.100f;
  const float charge_current =
      static_cast<float>(registers[REG_CHARGE_CURRENT_ADC - REG_STATUS] & 0x7F) * 0.050f;
  const float input_current_limit =
      0.100f + static_cast<float>(registers[REG_INPUT_CURRENT_ADC - REG_STATUS] & 0x3F) * 0.050f;
  const float battery_percent = estimate_battery_percent_(battery_voltage);

  if (battery_voltage_sensor_ != nullptr)
    battery_voltage_sensor_->publish_state(battery_voltage);
  if (system_voltage_sensor_ != nullptr)
    system_voltage_sensor_->publish_state(system_voltage);
  if (input_voltage_sensor_ != nullptr)
    input_voltage_sensor_->publish_state(input_voltage);
  if (charge_current_sensor_ != nullptr)
    charge_current_sensor_->publish_state(charge_current);
  if (input_current_limit_sensor_ != nullptr)
    input_current_limit_sensor_->publish_state(input_current_limit);
  if (battery_percent_sensor_ != nullptr)
    battery_percent_sensor_->publish_state(battery_percent);
  if (temperature_adc_sensor_ != nullptr)
    temperature_adc_sensor_->publish_state(temperature_adc * 100.0f);

  const bool charging = charge_status == 1 || charge_status == 2;
  const bool power_good = (status & 0x04) != 0;
  const bool battery_full = charge_status == 3;
  const bool boost_active = input_status == 7;
  const bool charger_fault = fault != 0;
  const bool battery_low = battery_percent <= warning_percent_;
  const bool battery_critical = battery_percent <= critical_percent_;
  const bool battery_shutdown = battery_percent <= shutdown_percent_;

  if (charging_sensor_ != nullptr)
    charging_sensor_->publish_state(charging);
  if (power_good_sensor_ != nullptr)
    power_good_sensor_->publish_state(power_good);
  if (battery_full_sensor_ != nullptr)
    battery_full_sensor_->publish_state(battery_full);
  if (boost_active_sensor_ != nullptr)
    boost_active_sensor_->publish_state(boost_active);
  if (charger_fault_sensor_ != nullptr)
    charger_fault_sensor_->publish_state(charger_fault);
  if (battery_low_sensor_ != nullptr)
    battery_low_sensor_->publish_state(battery_low);
  if (battery_critical_sensor_ != nullptr)
    battery_critical_sensor_->publish_state(battery_critical);
  if (battery_shutdown_sensor_ != nullptr)
    battery_shutdown_sensor_->publish_state(battery_shutdown);

  if (charge_status_sensor_ != nullptr)
    charge_status_sensor_->publish_state(charge_status_text_(charge_status));
  if (fault_status_sensor_ != nullptr) {
    const std::string fault_text = fault_status_text_(fault);
    fault_status_sensor_->publish_state(fault_text);
  }

  this->status_clear_warning();
}

bool BQ25895Component::configure_() {
  uint8_t input_source = 0;
  if (!this->read_register_(REG_INPUT_SOURCE, &input_source))
    return false;
  input_source = (input_source & ~INPUT_CURRENT_MASK) |
                 encode_input_current_limit_(input_current_limit_ma_);
  if (!this->write_register_(REG_INPUT_SOURCE, input_source))
    return false;

  uint8_t charge_current = 0;
  if (!this->read_register_(REG_CHARGE_CURRENT, &charge_current))
    return false;
  charge_current = (charge_current & ~CHARGE_CURRENT_MASK) |
                   encode_charge_current_(charge_current_ma_);
  if (!this->write_register_(REG_CHARGE_CURRENT, charge_current))
    return false;

  uint8_t charge_voltage = 0;
  if (!this->read_register_(REG_CHARGE_VOLTAGE, &charge_voltage))
    return false;
  charge_voltage = (charge_voltage & ~CHARGE_VOLTAGE_MASK) |
                   static_cast<uint8_t>(encode_voltage_limit_(charge_voltage_mv_) << 2);
  if (!this->write_register_(REG_CHARGE_VOLTAGE, charge_voltage))
    return false;

  uint8_t precharge_termination = 0;
  if (!this->read_register_(REG_PRECHARGE_TERMINATION, &precharge_termination))
    return false;
  precharge_termination =
      (precharge_termination & ~PRECHARGE_MASK) |
      static_cast<uint8_t>(encode_linear_current_(precharge_current_ma_) << 4);
  precharge_termination =
      (precharge_termination & ~TERMINATION_MASK) |
      encode_linear_current_(termination_current_ma_);
  if (!this->write_register_(REG_PRECHARGE_TERMINATION, precharge_termination))
    return false;

  return this->update_register_(
      REG_POWER_CONTROL, CHARGE_ENABLE_BIT,
      charge_enabled_ ? CHARGE_ENABLE_BIT : 0);
}

bool BQ25895Component::read_register_(uint8_t reg, uint8_t *value) {
  return this->read_register(reg, value, 1) == i2c::ERROR_OK;
}

bool BQ25895Component::write_register_(uint8_t reg, uint8_t value) {
  return this->write_register(reg, &value, 1) == i2c::ERROR_OK;
}

bool BQ25895Component::update_register_(uint8_t reg, uint8_t mask, uint8_t value) {
  uint8_t current = 0;
  if (!this->read_register_(reg, &current))
    return false;
  current = (current & ~mask) | (value & mask);
  return this->write_register_(reg, current);
}

bool BQ25895Component::read_status_block_(uint8_t *data) {
  return this->read_register(REG_STATUS, data, 10) == i2c::ERROR_OK;
}

uint8_t BQ25895Component::encode_input_current_limit_(uint16_t milliamps) {
  const uint16_t clamped = std::clamp<uint16_t>(milliamps, 100, 3250);
  return static_cast<uint8_t>(std::min<uint16_t>((clamped - 100 + 25) / 50, 63));
}

uint8_t BQ25895Component::encode_charge_current_(uint16_t milliamps) {
  const uint16_t clamped = std::min<uint16_t>(milliamps, 5056);
  return static_cast<uint8_t>(std::min<uint16_t>((clamped + 32) / 64, 79));
}

uint8_t BQ25895Component::encode_voltage_limit_(uint16_t millivolts) {
  const uint16_t clamped = std::clamp<uint16_t>(millivolts, 3840, 4608);
  return static_cast<uint8_t>(std::min<uint16_t>((clamped - 3840 + 8) / 16, 48));
}

uint8_t BQ25895Component::encode_linear_current_(uint16_t milliamps) {
  const uint16_t clamped = std::clamp<uint16_t>(milliamps, 64, 1024);
  return static_cast<uint8_t>(std::min<uint16_t>((clamped - 64 + 32) / 64, 15));
}

float BQ25895Component::estimate_battery_percent_(float voltage) {
  struct Point {
    float voltage;
    float percent;
  };

  static constexpr Point curve[] = {
      {3.30f, 0.0f},  {3.50f, 10.0f}, {3.60f, 20.0f}, {3.70f, 35.0f},
      {3.80f, 55.0f}, {3.90f, 75.0f}, {4.00f, 88.0f}, {4.10f, 96.0f},
      {4.20f, 100.0f},
  };

  if (voltage <= curve[0].voltage)
    return 0.0f;
  if (voltage >= curve[8].voltage)
    return 100.0f;

  for (size_t i = 1; i < 9; i++) {
    if (voltage <= curve[i].voltage) {
      const float span = curve[i].voltage - curve[i - 1].voltage;
      const float fraction = (voltage - curve[i - 1].voltage) / span;
      return curve[i - 1].percent +
             fraction * (curve[i].percent - curve[i - 1].percent);
    }
  }
  return 0.0f;
}

const char *BQ25895Component::charge_status_text_(uint8_t charge_status) {
  switch (charge_status) {
    case 1:
      return "Предзаряд";
    case 2:
      return "Быстрый заряд";
    case 3:
      return "Заряд завершён";
    default:
      return "Не заряжается";
  }
}

std::string BQ25895Component::fault_status_text_(uint8_t fault_register) {
  if (fault_register == 0)
    return "Ошибок нет";

  std::string result;
  auto append = [&result](const char *text) {
    if (!result.empty())
      result += ", ";
    result += text;
  };

  if (fault_register & 0x80)
    append("таймер I2C");
  if (fault_register & 0x40)
    append("ошибка boost");
  switch ((fault_register >> 4) & 0x03) {
    case 1:
      append("ошибка входа");
      break;
    case 2:
      append("перегрев");
      break;
    case 3:
      append("таймер зарядки");
      break;
    default:
      break;
  }
  if (fault_register & 0x08)
    append("ошибка аккумулятора");

  const uint8_t ntc_fault = fault_register & 0x07;
  if (ntc_fault == 1 || ntc_fault == 5)
    append("аккумулятор холодный");
  else if (ntc_fault == 2 || ntc_fault == 6)
    append("аккумулятор горячий");

  return result.empty() ? "Неизвестная ошибка" : result;
}

}  // namespace esphome::bq25895