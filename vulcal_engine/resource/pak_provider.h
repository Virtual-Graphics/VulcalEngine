#pragma once

#include "resource/resource_provider.h"

#include <filesystem>
#include <memory>

namespace vulcal::resource {

class PakProvider final : public ResourceProvider {
public:
    explicit PakProvider(std::filesystem::path pak_file);
    ~PakProvider() override;

    PakProvider(const PakProvider&)            = delete;
    PakProvider& operator=(const PakProvider&) = delete;
    PakProvider(PakProvider&&)                 = delete;
    PakProvider& operator=(PakProvider&&)      = delete;

    bool contains(std::string_view path) const override;
    std::vector<std::byte> read_binary(std::string_view path) const override;
    std::string read_text(std::string_view path) const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace vulcal::resource
