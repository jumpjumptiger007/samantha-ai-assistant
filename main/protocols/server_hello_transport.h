#pragma once

#include <cstring>

#include <cJSON.h>

namespace server_hello {

enum class Transport {
    kMissingOrNonString,
    kUnsupported,
    kWebSocket,
};

inline Transport ClassifyTransport(const cJSON* transport) {
    if (!cJSON_IsString(transport) || transport->valuestring == nullptr) {
        return Transport::kMissingOrNonString;
    }
    if (std::strcmp(transport->valuestring, "websocket") != 0) {
        return Transport::kUnsupported;
    }
    return Transport::kWebSocket;
}

}  // namespace server_hello
