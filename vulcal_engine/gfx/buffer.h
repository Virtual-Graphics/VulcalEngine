#pragma once
#include "object.h"
#include <vector>
#include <glad/glad.h>

namespace vulcal::gfx{
class Buffer : public Object {
public:
    enum class Usage {
        Static  = GL_STATIC_DRAW,
        Dynamic = GL_DYNAMIC_DRAW,
        Stream  = GL_STREAM_DRAW
    };

    Buffer() : Object([](GLuint id){ glDeleteBuffers(1, &id); }){
        glCreateBuffers(1, &id_);
    }
    virtual ~Buffer() = default;

    template<typename T>
    void set_data(const T* data, std::size_t count, Usage usage = Usage::Static){
        set_data(static_cast<const void*>(data), count * sizeof(T), usage);
    }

    template<typename T>
    void set_data(const std::vector<T>& data, Usage usage = Usage::Static){
        set_data(data.data(), data.size(), usage);
    }

    void set_data(const void* data, std::size_t size, Usage usage = Usage::Static){
        size_ = size;
        usage_ = usage;
        glNamedBufferData(id_, static_cast<GLsizeiptr>(size), data, static_cast<GLenum>(usage));
    }

    template<typename T>
    void set_sub_data(const T& data, std::size_t offset = 0) {
        set_sub_data(static_cast<const void*>(&data), sizeof(T), offset);
    }

    template<typename T>
    void set_sub_data(const T* data, std::size_t count, std::size_t offset = 0) {
        set_sub_data(static_cast<const void*>(data), count * sizeof(T), offset);
    }

    template<typename T>
    void set_sub_data(const std::vector<T>& data, std::size_t offset = 0) {
        set_sub_data(data.data(), data.size(), offset);
    }

    void set_sub_data(const void* data, std::size_t size, std::size_t offset = 0) {
        glNamedBufferSubData(id_, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
    }

    std::size_t size() const {
        return size_;
    }

private:
    std::size_t size_;
    Usage usage_ = Usage::Static;
};
}
