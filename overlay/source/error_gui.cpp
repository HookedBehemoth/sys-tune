#include "error_gui.hpp"

static char result_buffer[10];

ErrorGui::ErrorGui(const char *msg)
    : m_msg(msg) {}

ErrorGui::ErrorGui(Result rc) {
    std::snprintf(result_buffer, 10, "2%03d-%04d", R_MODULE(rc), R_DESCRIPTION(rc));
    m_msg = result_buffer;
}

tsl::elm::Element *ErrorGui::createUI() {
    auto rootFrame = new tsl::elm::OverlayFrame("Audioplayer \u266B", "v1.0.0");

    auto *custom = new tsl::elm::CustomDrawer([&](tsl::gfx::Renderer *drawer, u16 x, u16 y, u16 w, u16 h) {
        drawer->drawString("Something went wrong :/", false, 100, 240, 20, 0xffff);
        drawer->drawString(this->m_msg, false, 120, 400, 20, 0xffff);
    });

    rootFrame->setContent(custom);

    return rootFrame;
}

void ErrorGui::update() {}
bool ErrorGui::handleInput(u64, u64, touchPosition, JoystickPosition, JoystickPosition) {
    return false;
}
