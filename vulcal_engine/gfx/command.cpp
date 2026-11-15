#include "gfx/command.h"

#include "gfx/profiler.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace vulcal::gfx {

namespace {

GLenum to_gl(Primitive primitive) {
    switch (primitive) {
    case Primitive::Points:        return GL_POINTS;
    case Primitive::Lines:         return GL_LINES;
    case Primitive::LineStrip:     return GL_LINE_STRIP;
    case Primitive::LineLoop:      return GL_LINE_LOOP;
    case Primitive::Triangles:     return GL_TRIANGLES;
    case Primitive::TriangleStrip: return GL_TRIANGLE_STRIP;
    case Primitive::TriangleFan:   return GL_TRIANGLE_FAN;
    }
    return GL_TRIANGLES;
}

} // namespace

void Command::set_viewport(std::uint32_t width, std::uint32_t height) {
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

glm::ivec4 Command::viewport() {
    glm::ivec4 viewport{ 0 };
    glGetIntegerv(GL_VIEWPORT, glm::value_ptr(viewport));
    return viewport;
}

void Command::clear(const glm::vec4& color, float depth) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClearDepth(static_cast<GLdouble>(depth));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Command::clear_color(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Command::clear_depth(float depth) {
    glClearDepth(static_cast<GLdouble>(depth));
    glClear(GL_DEPTH_BUFFER_BIT);
}

void Command::apply(const PipelineState& pipeline) {
    pipeline.apply();
}

void Command::draw_indexed(const VertexArray& vao, Primitive primitive) {
    VULCAL_PROFILE_DRAW(vao.index_count());
    vao.bind();
    glDrawElements(to_gl(primitive), static_cast<GLsizei>(vao.index_count()), static_cast<GLenum>(vao.index_type()), nullptr);
}

} // namespace vulcal::gfx
