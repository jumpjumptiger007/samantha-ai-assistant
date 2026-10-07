#include "utils/input_parsing.h"
#include "protocols/server_hello_transport.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {

void TestPorts() {
    uint16_t port = 0;
    assert(input_parsing::ParsePort("1", port) && port == 1);
    assert(input_parsing::ParsePort("65535", port) && port == 65535);
    for (const std::string invalid : {"0", "65536", "-1", "", "123x"}) {
        assert(!input_parsing::ParsePort(invalid, port));
    }
}

void TestVersions() {
    using input_parsing::CompareVersions;
    using input_parsing::VersionComparison;

    assert(CompareVersions("2.5.0", "2.5.0") == VersionComparison::kEqualOrOlder);
    assert(CompareVersions("2.5.0", "2.5.1") == VersionComparison::kNewer);
    assert(CompareVersions("2.5.1", "2.6") == VersionComparison::kNewer);
    assert(CompareVersions("2.6", "3.0.0") == VersionComparison::kNewer);
    assert(CompareVersions("2.5.1", "2.5.0") == VersionComparison::kEqualOrOlder);

    for (const std::string invalid : {"", "2..5", "2.a.5", ".2", "2.", "2.5x", "version"}) {
        assert(CompareVersions(invalid, "2.5.0") == VersionComparison::kInvalidCurrent);
        assert(CompareVersions("2.5.0", invalid) == VersionComparison::kInvalidNew);
    }
}

void TestWebSocketTransport() {
    using server_hello::ClassifyTransport;
    using server_hello::Transport;
    assert(ClassifyTransport(nullptr) == Transport::kMissingOrNonString);

    cJSON null_value{};
    null_value.type = cJSON_NULL;
    assert(ClassifyTransport(&null_value) == Transport::kMissingOrNonString);

    cJSON number_value{};
    number_value.type = cJSON_Number;
    assert(ClassifyTransport(&number_value) == Transport::kMissingOrNonString);

    cJSON object_value{};
    object_value.type = cJSON_Object;
    assert(ClassifyTransport(&object_value) == Transport::kMissingOrNonString);

    char mqtt[] = "mqtt";
    cJSON wrong_string{};
    wrong_string.type = cJSON_String;
    wrong_string.valuestring = mqtt;
    assert(ClassifyTransport(&wrong_string) == Transport::kUnsupported);

    char websocket[] = "websocket";
    cJSON websocket_string{};
    websocket_string.type = cJSON_String;
    websocket_string.valuestring = websocket;
    assert(ClassifyTransport(&websocket_string) == Transport::kWebSocket);
}

}  // namespace

int main() {
    TestPorts();
    TestVersions();
    TestWebSocketTransport();
    return 0;
}
