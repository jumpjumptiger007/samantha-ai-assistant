#pragma once

#include <cstdint>

namespace launcher_compat {

struct PartitionTopology {
    bool running_is_ota_app;
    bool running_label_is_ota_slot;
    uint32_t running_address;
    uint32_t running_size;
    bool factory_present;
    uint32_t factory_address;
    uint32_t factory_size;
    bool otadata_present;
    uint32_t otadata_address;
    uint32_t otadata_size;
    bool assets_present;
};

// Match the verified 8 MB Launcher v1.5.0 topology. The canonical standalone
// Passport table has no factory app and uses low-address otadata plus assets.
constexpr bool IsLauncherChildLayout(const PartitionTopology& topology) {
    constexpr uint32_t kAlignment = 0x10000;
    constexpr uint32_t kFactoryAddress = 0x10000;
    constexpr uint32_t kFactorySize = 0x170000;
    constexpr uint32_t kOtadataAddress = 0x7fe000;
    constexpr uint32_t kOtadataSize = 0x2000;

    if (!topology.running_is_ota_app || !topology.running_label_is_ota_slot ||
        topology.running_size == 0 || !topology.factory_present || !topology.otadata_present ||
        topology.assets_present) {
        return false;
    }

    if (topology.factory_address != kFactoryAddress || topology.factory_size != kFactorySize ||
        topology.otadata_address != kOtadataAddress || topology.otadata_size != kOtadataSize ||
        topology.running_address % kAlignment != 0 || topology.running_size % kAlignment != 0) {
        return false;
    }

    const uint64_t factory_end =
        static_cast<uint64_t>(topology.factory_address) + topology.factory_size;
    const uint64_t running_end =
        static_cast<uint64_t>(topology.running_address) + topology.running_size;
    return topology.running_address >= factory_end && running_end <= topology.otadata_address;
}

// Cached, fail-closed detection of a Samantha app running as a Launcher Play.
bool IsLauncherChildMode();

}  // namespace launcher_compat
