#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace vulcal::resource {

class ResourceProvider {
public:
    virtual ~ResourceProvider()                                             = default;

    virtual bool contains(std::string_view path) const                      = 0;
    virtual std::vector<std::byte> read_binary(std::string_view path) const = 0;
    virtual std::string read_text(std::string_view path) const              = 0;
};

namespace detail {

std::string normalize_logical_path(std::string_view path);

} // namespace detail

} // namespace vulcal::resource
