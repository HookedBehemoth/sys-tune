#include "elm_seek_bar.hpp"

void SeekBar::draw(tsl::gfx::Renderer *renderer) {
    /* Progress */
    renderer->drawString(current_buffer, false, this->getX() + 15, this->getY() + 100 + 8, 20, 0xffff);
}
void SeekBar::layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) {
}
bool SeekBar::onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) {
}
bool SeekBar::onClick(u64 keys) {
}
tsl::elm::Element *SeekBar::requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) {
}
void SeekBar::setProgress(float progress) {
}
