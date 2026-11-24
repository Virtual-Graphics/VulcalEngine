#pragma once

#include <string>

namespace editor {

class Dialog {
public:
    virtual ~Dialog() = default;

    // Call from the same ImGui scope each frame.
    void draw();

protected:
    explicit Dialog(const char* title);

    void request_open();
    // Call from draw_contents() while the modal is active.
    void close();
    static void draw_inline_error(const std::string& error);

private:
    virtual void draw_contents() = 0;

    const std::string title_;
    bool open_requested_ = false;
};

} // namespace editor
