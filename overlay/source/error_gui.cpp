#include "error_gui.hpp"
#include "music_ovl_frame.hpp"

static char result_buffer[10];

ErrorGui::ErrorGui(const char *msg)
    : m_msg(msg), m_result() {}

ErrorGui::ErrorGui(const char *msg, Result rc) {
    std::snprintf(result_buffer, 10, "2%03d-%04d", R_MODULE(rc), R_DESCRIPTION(rc));
    m_result = result_buffer;
}

tsl::elm::Element *ErrorGui::createUI() {
    auto rootFrame = new MusicOverlayFrame("\uE0E1  Back     \uE0E0  OK");

    auto *custom = new tsl::elm::CustomDrawer([&](tsl::gfx::Renderer *drawer, u16 x, u16 y, u16 w, u16 h) {
        drawer->drawString("\uE150", false, (w - 90) / 2, 300, 90, 0xffff);
        drawer->drawString(this->m_msg, false, 55, 380, 25, 0xffff);
        if (this->m_result)
            drawer->drawString(this->m_result, false, 120, 430, 25, 0xffff);
    });

    rootFrame->setContent(custom);

    return rootFrame;
}

void ErrorGui::update() {}
bool ErrorGui::handleInput(u64, u64, touchPosition, JoystickPosition, JoystickPosition) {
    return false;
}
