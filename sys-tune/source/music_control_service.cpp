#include "music_control_service.hpp"

#include "impl/music_player.hpp"

namespace ams::music {

    Result ControlService::GetStatus(sf::Out<PlayerStatus> out) {
        return GetStatusImpl(out.GetPointer());
    }

    Result ControlService::SetStatus(PlayerStatus status) {
        return SetStatusImpl(status);
    }

    Result ControlService::GetVolume(sf::Out<double> out) {
        return GetVolumeImpl(out.GetPointer());
    }

    Result ControlService::SetVolume(double out) {
        return SetVolumeImpl(out);
    }

    Result ControlService::GetLoop(sf::Out<LoopStatus> out) {
        return GetLoopImpl(out.GetPointer());
    }
    Result ControlService::SetLoop(LoopStatus loop) {
        return SetLoopImpl(loop);
    }

    Result ControlService::GetCurrent(sf::OutBuffer out_path, sf::Out<CurrentTune> out_meta) {
        return GetCurrentImpl(reinterpret_cast<char *>(out_path.GetPointer()), out_path.GetSize(), out_meta.GetPointer());
    }

    Result ControlService::CountTunes(sf::Out<u32> out) {
        return CountTunesImpl(out.GetPointer());
    }

    Result ControlService::ListTunes(sf::OutBuffer out_path, sf::Out<u32> out) {
        return ListTunesImpl(reinterpret_cast<char *>(out_path.GetPointer()), out_path.GetSize(), out.GetPointer());
    }

    Result ControlService::AddToQueue(sf::InBuffer path) {
        return AddToQueueImpl(reinterpret_cast<const char *>(path.GetPointer()), path.GetSize());
    }

    Result ControlService::Clear() {
        return ClearImpl();
    }

    Result ControlService::Shuffle() {
        return ShuffleImpl();
    }

}
