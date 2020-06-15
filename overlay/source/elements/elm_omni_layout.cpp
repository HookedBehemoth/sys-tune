#include "elm_omni_layout.hpp"

void OmniDirectionalLayout::addElements(std::initializer_list<tsl::elm::Element *> elements) {
    // Add all elements
    for (auto &&elm : elements) {
        elm->setParent(this);
        this->m_elements.emplace_back(elm, DirectionReference());
    }

    //
    for (auto &[lhs, ref] : this->m_elements) {
        s32 left  = 0xffff;
        s32 right = 0xffff;
        s32 up    = 0xffff;
        s32 down  = 0xffff;
        for (auto &[rhs, _ref2] : this->m_elements) {
            if (lhs == rhs)
                continue;

            s32 diff = 0;

            // Vertical
            if (rhs->getX() + rhs->getWidth() >= lhs->getX() &&
                lhs->getX() + lhs->getWidth() >= rhs->getX()) {
                // tsl::FocusDirection::Up:
                diff = lhs->getY() - (rhs->getY() + rhs->getHeight());
                if (diff >= 0 && diff < up) {
                    ref.up = rhs.get();
                    up     = diff;
                }
                // tsl::FocusDirection::Down:
                diff = rhs->getY() - (lhs->getY() + lhs->getHeight());
                if (diff >= 0 && diff < down) {
                    ref.down = rhs.get();
                    down     = diff;
                }
            }
            // Horizontal
            if (rhs->getY() + rhs->getHeight() >= lhs->getY() &&
                lhs->getY() + lhs->getHeight() >= rhs->getY()) {
                // tsl::FocusDirection::Left:
                diff = lhs->getX() - (rhs->getX() + rhs->getWidth());
                if (diff >= 0 && diff < left) {
                    ref.left = rhs.get();
                    left     = diff;
                }
                // tsl::FocusDirection::Right:
                diff = rhs->getX() - (lhs->getX() + lhs->getHeight());
                if (diff >= 0 && diff < right) {
                    ref.right = rhs.get();
                    right     = diff;
                }
            }
        }
        std::printf("%p: up: %8d %11p, left: %8d %11p, down: %8d %11p, right: %8d %11p\n", lhs.get(), up, ref.up, left, ref.left, down, ref.down, right, ref.right);
    }
}

tsl::elm::Element *OmniDirectionalLayout::requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) {
    std::printf("old: %p, dir: %d\n", oldFocus, static_cast<int>(direction));

    if (direction == tsl::FocusDirection::None)
        return this->m_elements.front().first.get();

    auto &&last = std::find_if(this->m_elements.begin(), this->m_elements.end(), [oldFocus](std::pair<std::unique_ptr<tsl::elm::Element>, DirectionReference> &pair) {
        printf("old: %p, new: %p\n", oldFocus, pair.first.get());
        return pair.first.get() == oldFocus;
    });

    if (last == this->m_elements.end())
        return this->m_elements.empty() ? nullptr : this->m_elements.front().first.get();

    switch (direction) {
        case tsl::FocusDirection::Up:
            return last->second.up ? last->second.up->requestFocus(oldFocus, direction) : oldFocus;
        case tsl::FocusDirection::Down:
            return last->second.down ? last->second.down->requestFocus(oldFocus, direction) : oldFocus;
        case tsl::FocusDirection::Left:
            return last->second.left ? last->second.left->requestFocus(oldFocus, direction) : oldFocus;
        case tsl::FocusDirection::Right:
            return last->second.right ? last->second.right->requestFocus(oldFocus, direction) : oldFocus;
        default:
            return oldFocus;
    }
}

void OmniDirectionalLayout::draw(tsl::gfx::Renderer *renderer) {
    for (auto &[elm, _ref] : this->m_elements)
        elm->frame(renderer);
}

void OmniDirectionalLayout::layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) {
    for (auto &[elm, _ref] : this->m_elements)
        elm->layout(parentX, parentY, parentWidth, parentHeight);
}

bool OmniDirectionalLayout::onClick(u64 keys) {
    return false;
}

bool OmniDirectionalLayout::onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) {
    for (auto &[elm, _ref] : this->m_elements)
        if (elm->onTouch(event, currX, currY, prevX, prevY, initialX, initialY))
            return true;
    return false;
}
