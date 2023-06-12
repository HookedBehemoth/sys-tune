#include "pm.hpp"

namespace {

constexpr u64 QLAUNCH_TITLE_ID{0x0100000000001000ULL};
u64 CURRENT_TITLE_ID{};

// SOURCE: https://github.com/retronx-team/sys-clk/blob/570f1e5fe10b253eff0c8fda1bb893bb620af052/sysmodule/src/process_management.cpp#L37
auto GetCurrentTitleId() -> u64 {
    Result rc{};
    u64 pid{};
    u64 tid{};
    rc = pmdmntGetApplicationProcessId(&pid);

    if (rc == 0x20f) {
        return QLAUNCH_TITLE_ID;
    }

    if (R_FAILED(rc)) {
        return CURRENT_TITLE_ID;
    }

    rc = pminfoGetProgramId(&tid, pid);

    if (rc == 0x20f) {
        return QLAUNCH_TITLE_ID;
    }

    return tid;
}

} // namespace

namespace pm {

auto Initialize() -> Result {
    auto rc = pmdmntInitialize();
    if (R_FAILED(rc)) {
        return rc;
    }

    return pminfoInitialize();
}

void Exit() {
    pminfoExit();
    pmdmntExit();
}

auto PollCurrentTitle(u64& tid_out) -> bool {
    const auto new_tid = GetCurrentTitleId();
    if (new_tid != CURRENT_TITLE_ID) {
        CURRENT_TITLE_ID = new_tid;
        tid_out = new_tid;
        return true;
    }

    return false;
}

} // namespace pm
