#include "music_control_service.hpp"

#include "impl/music_player.hpp"

namespace ams::tune {

    Result ControlService::GetStatus(sf::Out<AudioOutState> out) {
        return impl::GetStatus(out.GetPointer());
    }

    Result ControlService::Play() {
        return impl::Play();
    }

    Result ControlService::Pause() {
        return impl::Pause();
    }

    Result ControlService::Next() {
        impl::Next();
        return ResultSuccess();
    }

    Result ControlService::Prev() {
        impl::Prev();
        return ResultSuccess();
    }

    Result ControlService::GetVolume(sf::Out<float> out) {
        return impl::GetVolume(out.GetPointer());
    }

    Result ControlService::SetVolume(float volume) {
        return impl::SetVolume(volume);
    }

    Result ControlService::GetRepeatMode(sf::Out<RepeatMode> mode) {
        impl::GetRepeatMode(mode.GetPointer());
        return ResultSuccess();
    }

    Result ControlService::SetRepeatMode(RepeatMode mode) {
        impl::SetRepeatMode(mode);
        return ResultSuccess();
    }

    Result ControlService::GetShuffleMode(sf::Out<ShuffleMode> mode) {
        impl::GetShuffleMode(mode.GetPointer());
        return ResultSuccess();
    }

    Result ControlService::SetShuffleMode(ShuffleMode mode) {
        impl::SetShuffleMode(mode);
        return ResultSuccess();
    }

    Result ControlService::GetCurrentPlaylistSize(sf::Out<u32> size) {
        impl::GetCurrentPlaylistSize(size.GetPointer());
        return ResultSuccess();
    }

    Result ControlService::GetCurrentPlaylist(sf::Out<u32> size, sf::OutBuffer buffer) {
        impl::GetCurrentPlaylist(size.GetPointer(), (char*)buffer.GetPointer(), buffer.GetSize());
        return ResultSuccess();
    }

    Result ControlService::GetCurrentQueueItem(sf::Out<CurrentStats> out, sf::OutBuffer buffer) {
        return impl::GetCurrentQueueItem(out.GetPointer(), (char*)buffer.GetPointer(), buffer.GetSize());
    }

    Result ControlService::ClearQueue() {
        impl::ClearQueue();
        return ResultSuccess();
    }

    Result ControlService::MoveQueueItem(u32 src, u32 dst) {
        impl::MoveQueueItem(src, dst);
        return ResultSuccess();
    }

    Result ControlService::Select(u32 index) {
        impl::Select(index);
        return ResultSuccess();
    }

    Result ControlService::Enqueue(sf::InBuffer buffer, EnqueueType type) {
        return impl::Enqueue((char*)buffer.GetPointer(), buffer.GetSize(), type);
    }

    Result ControlService::Remove(u32 index) {
        return impl::Remove(index);
    }

}
