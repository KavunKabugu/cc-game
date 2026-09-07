#ifndef CC_MAP_CONVERTER_INTRALISM_PARSE_H
#define CC_MAP_CONVERTER_INTRALISM_PARSE_H

#include <optional>
#include <string_view>
#include <vector>

namespace MapConverter {

// Compass lanes match cc-game: Left=0, Up=1, Right=2, Down=3.
[[nodiscard]] inline std::string_view TrimIntralismToken(const std::string_view s) {
    const size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    const size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

[[nodiscard]] inline std::optional<int> IntralismDirectionToLane(const std::string_view direction) {
    const std::string_view d = TrimIntralismToken(direction);
    if (d == "Left") {
        return 0;
    }
    if (d == "Up") {
        return 1;
    }
    if (d == "Right") {
        return 2;
    }
    if (d == "Down") {
        return 3;
    }
    return std::nullopt;
}

// Parse SpawnObj data[1] payloads such as "[Up,False]", "[Left-Right,False]", "[Left-Up]".
// Unknown direction tokens are skipped. Missing brackets yield no lanes.
[[nodiscard]] inline std::vector<int> ParseSpawnObjLanes(const std::string_view payload) {
    std::vector<int> lanes;
    const size_t open = payload.find('[');
    if (open == std::string_view::npos) {
        return lanes;
    }

    const std::string_view after = payload.substr(open + 1);
    const size_t endComma = after.find(',');
    const size_t endBracket = after.find(']');
    size_t end = after.size();
    if (endComma != std::string_view::npos && endBracket != std::string_view::npos) {
        end = (endComma < endBracket) ? endComma : endBracket;
    } else if (endComma != std::string_view::npos) {
        end = endComma;
    } else if (endBracket != std::string_view::npos) {
        end = endBracket;
    }

    const std::string_view inner = after.substr(0, end);
    size_t start = 0;
    while (start <= inner.size()) {
        const size_t dash = inner.find('-', start);
        const std::string_view token =
            (dash == std::string_view::npos) ? inner.substr(start) : inner.substr(start, dash - start);
        if (const auto lane = IntralismDirectionToLane(token)) {
            lanes.push_back(*lane);
        }
        if (dash == std::string_view::npos) {
            break;
        }
        start = dash + 1;
    }
    return lanes;
}

} // namespace MapConverter

#endif // CC_MAP_CONVERTER_INTRALISM_PARSE_H
