#include "music_control_service.hpp"

#include "impl/music_player.hpp"

namespace ams::music {

    Result ControlService::GetStatus(sf::Out<PlayerStatus> out) {
        return GetStatusImpl(out.GetPointer());
    }

    Result ControlService::SetStatus(PlayerStatus status) {
        return SetStatusImpl(status);
    }

    Result ControlService::GetQueueCount(sf::Out<u32> out) {
        return GetQueueCountImpl(out.GetPointer());
    }

    Result ControlService::GetCurrent(sf::OutBuffer out_path) {
        return GetCurrentImpl(reinterpret_cast<char *>(out_path.GetPointer()), out_path.GetSize());
    }

    Result ControlService::GetList(sf::OutBuffer out_path, sf::Out<u32> out) {
        return GetListImpl(reinterpret_cast<char *>(out_path.GetPointer()), out_path.GetSize(), out.GetPointer());
    }

    Result ControlService::AddToQueue(sf::InBuffer path) {
        return AddToQueueImpl(reinterpret_cast<const char *>(path.GetPointer()), path.GetSize());
    }

    Result ControlService::Clear() {
        return ClearImpl();
    }

}
