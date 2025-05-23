#pragma once

#include <string>
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "SNMP_Agent.h"

#ifdef USE_ESP32
#include <WiFi.h>
#include <esp32/himem.h>
#endif
#ifdef USE_ESP8266
#include <ESP8266WiFi.h>
#endif
#include <WiFiUdp.h>

namespace esphome {
namespace snmp {

class SNMPComponent : public Component {
 public:
  SNMPComponent() : snmp_agent_("public", "private") {}

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
  void loop() override;

  void set_contact(const std::string &contact) { contact_ = contact; }
  void set_location(const std::string &location) { location_ = location; }

  void set_sensor_values(float temperature, float humidity);

 protected:
  WiFiUDP udp_;
  SNMPAgent snmp_agent_;

  void setup_system_mib_();
  void setup_storage_mib_();
  void setup_chip_mib_();
  void setup_wifi_mib_();
  void setup_sensor_mib_();

#ifdef USE_ESP32
  void setup_esp32_heap_mib_();
  static int setup_psram_size(int *used);
  static int get_ram_size_kb();
#endif

#ifdef USE_ESP8266
  void setup_esp8266_heap_mib_();
#endif

  static uint32_t get_uptime();
  static uint32_t get_net_uptime();
  static std::string get_bssid();

  std::string contact_;              
  std::string location_;

  // Variáveis estáticas para manter valores dos sensores
  static float temperature_;
  static float humidity_;

  // Métodos estáticos compatíveis com GETINT_FUNC
  static int get_temperature_int(); 
  static int get_humidity_int();

};

}  // namespace snmp
}  // namespace esphome
