#include "render/pipeline/pass.h"

#include "base/logger.h"
#include "gfx/command.h"
#include "gfx/shader.h"

namespace vulcal::render {

Pass::Pass(const gfx::PipelineState::Spec& pipeline_spec) : pipeline_state_(pipeline_spec) {}

void Pass::load_shaders(const char* vertex_source, const char* fragment_source) {
    base::log::debug("Compiling embedded shaders for pass '{}'", name());
    gfx::Shader vertex_shader(gfx::Shader::Type::Vertex, vertex_source);
    gfx::Shader fragment_shader(gfx::Shader::Type::Fragment, fragment_source);
    program_.attach(vertex_shader);
    program_.attach(fragment_shader);
    program_.link();
}

void Pass::bind_pipeline() {
    gfx::PipelineState::reset_cache();
    gfx::Command::apply(pipeline_state_);
    program_.use();
}

} // namespace vulcal::render
