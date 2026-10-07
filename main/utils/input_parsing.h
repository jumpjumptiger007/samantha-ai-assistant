#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>
#include <system_error>
#include <vector>

namespace input_parsing {

inline bool ParsePort(std::string_view text, uint16_t& port) {
    unsigned int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, 10);
    if (text.empty() || error != std::errc{} || end != text.data() + text.size() || value == 0 ||
        value > UINT16_MAX) {
        return false;
    }

    port = static_cast<uint16_t>(value);
    return true;
}

inline bool ParseVersion(std::string_view text, std::vector<uint32_t>& version) {
    version.clear();
    if (text.empty()) {
        return false;
    }

    size_t start = 0;
    while (start < text.size()) {
        const size_t end = text.find('.', start);
        const size_t segment_end = end == std::string_view::npos ? text.size() : end;
        if (segment_end == start) {
            version.clear();
            return false;
        }
        uint32_t value = 0;
        const auto [parsed_end, error] =
            std::from_chars(text.data() + start, text.data() + segment_end, value, 10);
        if (parsed_end != text.data() + segment_end || error != std::errc{}) {
            version.clear();
            return false;
        }

        version.push_back(value);
        if (end == std::string_view::npos) {
            return true;
        }
        start = end + 1;
    }

    version.clear();
    return false;
}

enum class VersionComparison {
    kInvalidCurrent,
    kInvalidNew,
    kEqualOrOlder,
    kNewer,
};

inline VersionComparison CompareVersions(std::string_view current_text, std::string_view new_text) {
    std::vector<uint32_t> current;
    if (!ParseVersion(current_text, current)) {
        return VersionComparison::kInvalidCurrent;
    }

    std::vector<uint32_t> newer;
    if (!ParseVersion(new_text, newer)) {
        return VersionComparison::kInvalidNew;
    }

    const size_t shared_segments = current.size() < newer.size() ? current.size() : newer.size();
    for (size_t i = 0; i < shared_segments; ++i) {
        if (newer[i] > current[i]) {
            return VersionComparison::kNewer;
        }
        if (newer[i] < current[i]) {
            return VersionComparison::kEqualOrOlder;
        }
    }

    return newer.size() > current.size() ? VersionComparison::kNewer
                                         : VersionComparison::kEqualOrOlder;
}

}  // namespace input_parsing
