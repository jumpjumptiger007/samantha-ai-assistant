#include "launcher_compat.h"

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#include <cstring>

#define TAG "LauncherCompat"

namespace launcher_compat {
namespace {

bool IsOtaSlotLabel(const char* label, unsigned expected_slot) {
    if (label == nullptr || std::strncmp(label, "ota_", 4) != 0 || label[4] == '\0') {
        return false;
    }

    unsigned slot = 0;
    for (const char* digit = label + 4; *digit != '\0'; ++digit) {
        if (*digit < '0' || *digit > '9') {
            return false;
        }
        slot = slot * 10 + static_cast<unsigned>(*digit - '0');
        if (slot > 15) {
            return false;
        }
    }
    return slot == expected_slot;
}

bool DetectLauncherChildMode() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* factory = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "factory");
    const esp_partition_t* otadata = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, "otadata");
    const esp_partition_t* assets =
        esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, "assets");

    const bool running_is_ota = running != nullptr && running->type == ESP_PARTITION_TYPE_APP &&
                                running->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MIN &&
                                running->subtype < ESP_PARTITION_SUBTYPE_APP_OTA_MAX;
    const unsigned running_slot =
        running_is_ota ? static_cast<unsigned>(running->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_MIN)
                       : 16;

    PartitionTopology topology = {
        .running_is_ota_app = running_is_ota,
        .running_label_is_ota_slot =
            running != nullptr && running_is_ota && IsOtaSlotLabel(running->label, running_slot),
        .running_address = running != nullptr ? running->address : 0,
        .running_size = running != nullptr ? running->size : 0,
        .factory_present = factory != nullptr,
        .factory_address = factory != nullptr ? factory->address : 0,
        .factory_size = factory != nullptr ? factory->size : 0,
        .otadata_present = otadata != nullptr,
        .otadata_address = otadata != nullptr ? otadata->address : 0,
        .otadata_size = otadata != nullptr ? otadata->size : 0,
        .assets_present = assets != nullptr,
    };
    return IsLauncherChildLayout(topology);
}

}  // namespace

bool IsLauncherChildMode() {
    static const bool is_launcher_child = DetectLauncherChildMode();
    static const bool logged = []() {
        ESP_LOGI(TAG, "Launcher child mode: %s", is_launcher_child ? "true" : "false");
        return true;
    }();
    (void)logged;
    return is_launcher_child;
}

}  // namespace launcher_compat
