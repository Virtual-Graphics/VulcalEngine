#pragma once

#include "resource/resource_provider.h"

#include <filesystem>

namespace vulcal::resource {

class DirectoryProvider final : public ResourceProvider {
public:
    explicit DirectoryProvider(std::filesystem::path root);

    bool contains(std::string_view path) const override;
    std::vector<std::byte> read_binary(std::string_view path) const override;
    std::string read_text(std::string_view path) const override;

    const std::filesystem::path& root() const { 
        return root_; 
    }

private:
    std::filesystem::path resolve(std::string_view path) const;

    std::filesystem::path root_;
};

} // namespace vulcal::resource
