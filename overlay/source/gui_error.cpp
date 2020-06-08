#include "gui_error.hpp"

static char result_buffer[10];

ErrorGui::ErrorGui(const char *msg, Result rc) : m_msg(msg) {
    if (rc) {
        std::snprintf(result_buffer, 10, "2%03d-%04d", R_MODULE(rc), R_DESCRIPTION(rc));
        m_result = result_buffer;
    }
}

tsl::elm::Element *ErrorGui::createUI() {
    auto rootFrame = new tsl::elm::OverlayFrame("ovl-tune \u266B", VERSION);

    auto *custom = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *drawer, u16 x, u16 y, u16 w, u16 h) {
        drawer->drawString("\uE150", false, x + (w / 2) - (90 / 2), 300, 90, 0xffff);
        auto [width, height] = drawer->drawString(this->m_msg, false, x + (w / 2) - (this->msgW / 2), 380, 25, 0xffff);
        if (msgW == 0) {
            msgW = width;
            msgH = height;
        }
        if (this->m_result)
            drawer->drawString(this->m_result, false, 120, 430, 25, 0xffff);
    });

    rootFrame->setContent(custom);

    return rootFrame;
}
