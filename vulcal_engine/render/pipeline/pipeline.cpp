#include "render/pipeline/pipeline.h"

#include "gfx/command.h"
#include "gfx/debug.h"
#include "render/pipeline/forward_pass.h"
#include "render/pipeline/shadow_pass.h"
#include "render/pipeline/sprite_pass.h"
#include "render/render_data.h"

namespace vulcal::render {

Pipeline::Pipeline() {
    passes_.push_back(std::make_unique<ShadowPass>());
    passes_.push_back(std::make_unique<ForwardPass>());
    passes_.push_back(std::make_unique<SpritePass>());
}

void Pipeline::render(const RenderData& data, gfx::RenderTarget* target,
        const std::vector<DrawItem>& items) {
    PassContext ctx{ .data_ = data, .target_ = target, .viewport_ = gfx::Command::viewport() };

    for (const auto& pass: passes_) {
        if (!pass->enabled(data)) {
            continue;
        }

        gfx::DebugScope scope(pass->name());
        pass->prepare(ctx);
        for (const DrawItem& item: items) {
            pass->draw(ctx, item);
        }
        pass->finish(ctx);
    }
}

} // namespace vulcal::render
