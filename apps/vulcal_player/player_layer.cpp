#include "vulcal_player/player_layer.h"

#include "asset/gpu_resource_registry.h"
#include "base/logger.h"
#include "base/path_utils.h"
#include "engine/application.h"
#include "gfx/command.h"
#include "gfx/render_target.h"
#include "render/renderer.h"
#include "resource/pak_provider.h"
#include "scene/scene_serializer.h"

namespace vulcal::player {

PlayerLayer::PlayerLayer(std::filesystem::path pak_file)
        : engine::Layer("PlayerLayer"),
          pak_file_(std::move(pak_file)) {}

PlayerLayer::~PlayerLayer() { on_detach(); }

void PlayerLayer::on_attach() {
    resources_ = std::make_shared<resource::PakProvider>(pak_file_);
    manifest_  = load_manifest(*resources_);
    scene_     = scene::SceneSerializer::deserialize(*resources_, manifest_.open_scene);
    assets_.set_provider(resources_, ".vulcal_engine");
    renderer_      = std::make_unique<render::Renderer>();
    gpu_resources_ = std::make_unique<asset::GPUResourceRegistry>(assets_, *renderer_);
    extractor_     = std::make_unique<scene::SceneExtractor>(*gpu_resources_);
    scripts_.start(*scene_, *resources_);
    base::log::info("Loaded game '{}' from {}", manifest_.name, base::path_to_utf8(pak_file_));
}

void PlayerLayer::on_detach() {
    scripts_.stop();
    extractor_.reset();
    gpu_resources_.reset();
    renderer_.reset();
    assets_.clear();
    scene_.reset();
    resources_.reset();
}

void PlayerLayer::on_update(base::Timestep timestep) { 
    scripts_.update(timestep.get_seconds()); 
}

void PlayerLayer::on_render() {
    if (!scene_ || !renderer_ || !extractor_) {
        return;
    }

    const auto& window = application().get_window();
    if (window.width() == 0 || window.height() == 0) {
        return;
    }
    
    renderer_->render(extractor_->extract(*scene_, static_cast<float>(window.width()) / static_cast<float>(window.height())));
    vulcal::gfx::RenderTarget::unbind();
    vulcal::gfx::Command::set_viewport(window.width(), window.height());
}

} // namespace vulcal::player
