#include "snmp_component.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/version.h"
#include "esphome/components/wifi/wifi_component.h"

namespace esphome {
namespace snmp {

#define CUSTOM_OID ".1.3.9999."
#define CUSTOM_SENSOR_OID CUSTOM_OID "5."

static const char *const TAG = "snmp";

// Variáveis globais para sensores
static float g_temperature = NAN;
static float g_humidity = NAN;

int SNMPComponent::get_temperature_int() {
  return isnan(g_temperature) ? -9999 : static_cast<int>(g_temperature * 10);
}

int SNMPComponent::get_humidity_int() {
  return isnan(g_humidity) ? -9999 : static_cast<int>(g_humidity * 10);
}

uint32_t SNMPComponent::get_net_uptime() {
#ifdef WIFI_CONNECTED_TIMESTAMP_AVAILABLE
  return (millis() - wifi::global_wifi_component->wifi_connected_timestamp()) / 10;
#else
  return 0;
#endif
}

void SNMPComponent::setup_system_mib_() {
  const char *desc_fmt = "ESPHome version " ESPHOME_VERSION " compiled %s, Board " ESPHOME_BOARD;
  char description[128];
  snprintf(description, sizeof(description), desc_fmt, App.get_compilation_time().c_str());
  snmp_agent_.addReadOnlyStaticStringHandler(RFC1213_OID_sysDescr, description);

  snmp_agent_.addDynamicReadOnlyStringHandler(RFC1213_OID_sysName, []() -> std::string { return App.get_name(); });
  snmp_agent_.addReadOnlyIntegerHandler(RFC1213_OID_sysServices, 64);

  snmp_agent_.addOIDHandler(RFC1213_OID_sysObjectID,
#ifdef USE_ESP32
                            CUSTOM_OID "32"
#else
                            CUSTOM_OID "8266"
#endif
  );

  snmp_agent_.addReadOnlyStaticStringHandler(RFC1213_OID_sysContact, contact_);
  snmp_agent_.addReadOnlyStaticStringHandler(RFC1213_OID_sysLocation, location_);
}

#ifdef USE_ESP32
int SNMPComponent::setup_psram_size(int *used) {
  int total_size = 0;
  *used = 0;

  size_t free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  if (free_size > 0) {
    total_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    if (total_size > 0) {
      *used = total_size - free_size;
    }
  }

  return total_size;
}
#endif

void SNMPComponent::setup_storage_mib_() {
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.1.1", 1);
  snmp_agent_.addReadOnlyStaticStringHandler(".1.3.6.1.2.1.25.2.3.1.3.1", "FLASH");
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.4.1", 1);
  snmp_agent_.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.1", []() -> int { return ESP.getFlashChipSize(); });
  snmp_agent_.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.1", []() -> int { return ESP.getSketchSize(); });

  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.1.2", 2);
  snmp_agent_.addReadOnlyStaticStringHandler(".1.3.6.1.2.1.25.2.3.1.3.2", "SPI RAM");
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.4.2", 1);

#ifdef USE_ESP32
  snmp_agent_.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.2", []() -> int {
    int u;
    return setup_psram_size(&u);
  });
  snmp_agent_.addDynamicIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.2", []() -> int {
    int u;
    setup_psram_size(&u);
    return u;
  });
#endif

#ifdef USE_ESP8266
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.5.2", 0);
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.3.1.6.2", 0);
#endif

#ifdef USE_ESP32
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.2", get_ram_size_kb());
#endif
#ifdef USE_ESP8266
  snmp_agent_.addReadOnlyIntegerHandler(".1.3.6.1.2.1.25.2.2", 160);
#endif
}

std::string SNMPComponent::get_bssid() {
  char buf[30];
  wifi::bssid_t bssid = wifi::global_wifi_component->wifi_bssid();
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  return buf;
}

#ifdef USE_ESP32
void SNMPComponent::setup_esp32_heap_mib_() {
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "32.1.0", []() -> int { return ESP.getHeapSize(); });
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "32.2.0", []() -> int { return ESP.getFreeHeap(); });
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "32.3.0", []() -> int { return ESP.getMinFreeHeap(); });
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "32.4.0", []() -> int { return ESP.getMaxAllocHeap(); });
}
#endif

#ifdef USE_ESP8266
void SNMPComponent::setup_esp8266_heap_mib_() {
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "8266.1.0", []() -> int { return ESP.getFreeHeap(); });
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "8266.2.0", []() -> int { return ESP.getHeapFragmentation(); });
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "8266.3.0", []() -> int { return ESP.getMaxFreeBlockSize(); });
}
#endif

void SNMPComponent::setup_chip_mib_() {
#ifdef USE_ESP32
  snmp_agent_.addReadOnlyIntegerHandler(CUSTOM_OID "2.1.0", 32);
#endif
#ifdef USE_ESP8266
  snmp_agent_.addReadOnlyIntegerHandler(CUSTOM_OID "2.1.0", 8266);
#endif

  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "2.2.0", []() -> int { return ESP.getCpuFreqMHz(); });

#ifdef USE_ESP32
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "2.3.0", []() -> std::string { return ESP.getChipModel(); });
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "2.4.0", []() -> int { return ESP.getChipCores(); });
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "2.5.0", []() -> int { return ESP.getChipRevision(); });
#endif
#ifdef USE_ESP8266
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "2.3.0", []() -> std::string { return ESP.getCoreVersion().c_str(); });
  snmp_agent_.addReadOnlyIntegerHandler(CUSTOM_OID "2.4.0", 1);
  snmp_agent_.addReadOnlyIntegerHandler(CUSTOM_OID "2.5.0", 0);
#endif
}

void SNMPComponent::setup_wifi_mib_() {
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_OID "4.1.0", []() -> int { return wifi::global_wifi_component->wifi_rssi(); });
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "4.2.0", get_bssid);
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "4.3.0", []() -> std::string { return wifi::global_wifi_component->wifi_ssid(); });
  snmp_agent_.addDynamicReadOnlyStringHandler(CUSTOM_OID "4.4.0", []() -> std::string {
    const auto& ip_array = wifi::global_wifi_component->wifi_sta_ip_addresses();
    return ip_array.size() ? ip_array[0].str() : "";
  });
}

void SNMPComponent::setup_sensor_mib_() {
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_SENSOR_OID "1.0", SNMPComponent::get_temperature_int);
  snmp_agent_.addDynamicIntegerHandler(CUSTOM_SENSOR_OID "2.0", SNMPComponent::get_humidity_int);
}

void SNMPComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SNMP...");

  snmp_agent_.addDynamicReadOnlyTimestampHandler(RFC1213_OID_sysUpTime, get_net_uptime);
  snmp_agent_.addDynamicReadOnlyTimestampHandler(".1.3.6.1.2.1.25.1.1.0", get_uptime);

  setup_system_mib_();
  setup_storage_mib_();
#ifdef USE_ESP32
  setup_esp32_heap_mib_();
#endif
#ifdef USE_ESP8266
  setup_esp8266_heap_mib_();
#endif
  setup_chip_mib_();
  setup_wifi_mib_();
  setup_sensor_mib_();

  snmp_agent_.sortHandlers();
  snmp_agent_.setUDP(&udp_);
  snmp_agent_.begin();
}

void SNMPComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SNMP Config:");
  ESP_LOGCONFIG(TAG, "  Contact: \"%s\"", contact_.c_str());
  ESP_LOGCONFIG(TAG, "  Location: \"%s\"", location_.c_str());
}

void SNMPComponent::loop() {
  snmp_agent_.loop();
}

#ifdef USE_ESP32
int SNMPComponent::get_ram_size_kb() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  switch ((int) chip_info.model) {
    case 1: return 520;  // ESP32
    case 2: return 320;  // S2
    case 9: return 512;  // S3
    case 5: return 400;  // C3
    case 6: return 256;  // H2
    case 12:
    case 13: return 400; // C2/C6
  }
  return 0;
}
#endif

void SNMPComponent::set_sensor_values(float temperature, float humidity) {
  g_temperature = temperature;
  g_humidity = humidity;
}

}  // namespace snmp
}  // namespace esphome
