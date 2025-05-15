#include "snmp_component.h"

namespace esphome {
namespace snmp {

// Inicialização das variáveis estáticas
float SNMPComponent::temperature_ = NAN;
float SNMPComponent::humidity_ = NAN;

void SNMPComponent::setup() {
  snmp_agent_.begin(161, &udp_);
  this->setup_system_mib_();
  this->setup_storage_mib_();
  this->setup_chip_mib_();
  this->setup_wifi_mib_();
  this->setup_sensor_mib_();
}

void SNMPComponent::loop() {
  snmp_agent_.listen();
}

void SNMPComponent::dump_config() {
  ESP_LOGCONFIG("snmp", "SNMP Component:");
  ESP_LOGCONFIG("snmp", "  Contact: %s", contact_.c_str());
  ESP_LOGCONFIG("snmp", "  Location: %s", location_.c_str());
}

void SNMPComponent::set_sensor_values(float temperature, float humidity) {
  temperature_ = temperature;
  humidity_ = humidity;
}

// SNMP sensor handler functions
int SNMPComponent::get_temperature_int() {
  return isnan(temperature_) ? -9999 : static_cast<int>(temperature_ * 10);
}

int SNMPComponent::get_humidity_int() {
  return isnan(humidity_) ? -9999 : static_cast<int>(humidity_ * 10);
}

// MIB com sensores
void SNMPComponent::setup_sensor_mib_() {
  snmp_agent_.addDynamicIntegerHandler("1.3.6.1.4.1.53864.1.0", get_temperature_int); // temperature
  snmp_agent_.addDynamicIntegerHandler("1.3.6.1.4.1.53864.2.0", get_humidity_int);    // humidity
}

// ... (outros métodos como setup_system_mib_, etc., não alterados)

}  // namespace snmp
}  // namespace esphome
