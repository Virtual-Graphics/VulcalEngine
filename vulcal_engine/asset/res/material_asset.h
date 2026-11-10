#pragma once

#include "render/resources/material.h"

#include <filesystem>

namespace vulcal::asset {

class MaterialAsset {
public:
    static render::Material read(const std::filesystem::path& path);
    static void write(const std::filesystem::path& path, const render::Material& material);
};

} // namespace vulcal::asset
