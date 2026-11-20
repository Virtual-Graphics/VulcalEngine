#include "resource/resource_provider.h"

#include <cstdint>
#include <stdexcept>

namespace vulcal::resource::detail {
namespace {

//校验一段文本是不是合法的可作为路径使用的 UTF-8 字符串
bool is_valid_path_text(std::string_view text) {
    for (std::size_t i = 0; i < text.size();) {
        const auto first = static_cast<unsigned char>(text[i]);
        if (first <= 0x7f) {
            if (first < 0x20 || first == 0x7f) { 
                return false;
            }
            ++i;
            continue;
        }

        std::uint32_t codepoint = 0;
        std::size_t continuation_count = 0;

        if ((first & 0xe0) == 0xc0) {
            codepoint = first & 0x1f;
            continuation_count = 1;
            if (codepoint < 2) { 
                return false;
            }
        } else if ((first & 0xf0) == 0xe0) {
            codepoint = first & 0x0f;
            continuation_count = 2;
        } else if ((first & 0xf8) == 0xf0) {
            codepoint = first & 0x07;
            continuation_count = 3;
        } else {
            return false;
        }

        if (i + continuation_count >= text.size()) { 
            return false;
        }

        for (std::size_t j = 1; j <= continuation_count; ++j) {
            const auto next = static_cast<unsigned char>(text[i + j]);
            if ((next & 0xc0) != 0x80) { 
                return false;
            }
            codepoint = (codepoint << 6) | (next & 0x3f);
        }

        if ((continuation_count == 2 && codepoint < 0x800) || (continuation_count == 3 && codepoint < 0x10000) || codepoint > 0x10ffff ||
            (codepoint >= 0xd800 && codepoint <= 0xdfff) || (codepoint >= 0x80 && codepoint <= 0x9f)) {
            return false;
        }
        i += continuation_count + 1;
    }
    return true;
}

} // namespace

std::string normalize_logical_path(std::string_view path) {
    if (path.empty() ||
        path.front() == '/' ||
        path.back() == '/' ||
        path.find('\\') != std::string_view::npos ||
        path.find(':') != std::string_view::npos ||
        !is_valid_path_text(path)) {
        throw std::invalid_argument("Resource path must be a UTF-8 relative path using '/' separators and no control characters");
    }

    std::size_t component_start = 0;
    while (component_start < path.size()) {
        const auto separator = path.find('/', component_start);
        const auto end       = separator == std::string_view::npos ? path.size() : separator;
        const auto component = path.substr(component_start, end - component_start);

        if (component.empty() || component == "." || component == "..") {
            throw std::invalid_argument("Resource path contains an empty or traversal component");
        }

        component_start = end + 1;
    }
    return std::string(path);
}

} // namespace vulcal::resource::detail
