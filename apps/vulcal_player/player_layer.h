#pragma once

#include "asset/asset_manager.h"
#include "engine/layer.h"
#include "player_runtime.h"
#include "scene/scene.h"
#include "scene/scene_extractor.h"
#include "script/script_runtime.h"

#include <filesystem>
#include <memory>

namespace vulcal::asset {
class GPUResourceRegistry;
}
namespace vulcal::render {
class Renderer;
}
namespace vulcal::resource {
class PakProvider;
}

namespace vulcal::player {

class PlayerLayer final : public engine::Layer {
public:
    explicit PlayerLayer(std::filesystem::path pak_file);
    ~PlayerLayer() override;
    void on_attach() override;
    void on_detach() override;
    void on_update(base::Timestep timestep) override;
    void on_render() override;

private:
    std::filesystem::path pak_file_;
    std::shared_ptr<resource::PakProvider> resources_;
    Manifest manifest_;
    std::unique_ptr<scene::Scene> scene_;
    asset::AssetManager assets_;
    script::ScriptRuntime scripts_;
    std::unique_ptr<render::Renderer> renderer_;
    std::unique_ptr<asset::GPUResourceRegistry> gpu_resources_;
    std::unique_ptr<scene::SceneExtractor> extractor_;
};

} // namespace vulcal::player
