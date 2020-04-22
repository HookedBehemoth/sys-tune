#include "music_control_service.hpp"

#include "impl/music_player.hpp"

namespace ams::tune {

    Result ControlService::GetStatus(sf::Out<bool> out) {
        out.SetValue(impl::GetStatus());
        return ResultSuccess();
    }

    Result ControlService::Play() {
        impl::Play();
        return ResultSuccess();
    }

    Result ControlService::Pause() {
        impl::Pause();
        return ResultSuccess();
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
        out.SetValue(impl::GetVolume());
        return ResultSuccess();
    }

    Result ControlService::SetVolume(float volume) {
        impl::SetVolume(volume);
        return ResultSuccess();
    }

    Result ControlService::GetRepeatMode(sf::Out<RepeatMode> mode) {
        mode.SetValue(impl::GetRepeatMode());
        return ResultSuccess();
    }

    Result ControlService::SetRepeatMode(RepeatMode mode) {
        impl::SetRepeatMode(mode);
        return ResultSuccess();
    }

    Result ControlService::GetShuffleMode(sf::Out<ShuffleMode> mode) {
        mode.SetValue(impl::GetShuffleMode());
        return ResultSuccess();
    }

    Result ControlService::SetShuffleMode(ShuffleMode mode) {
        impl::SetShuffleMode(mode);
        return ResultSuccess();
    }

    Result ControlService::GetCurrentPlaylistSize(sf::Out<u32> size) {
        size.SetValue(impl::GetCurrentPlaylistSize());
        return ResultSuccess();
    }

    Result ControlService::GetCurrentPlaylist(sf::Out<u32> size, sf::OutBuffer buffer) {
        size.SetValue(impl::GetCurrentPlaylist((char*)buffer.GetPointer(), buffer.GetSize()));
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

    Result ControlService::Seek(u32 position) {
        impl::Seek(position);
        return ResultSuccess();
    }

    Result ControlService::Enqueue(sf::InBuffer buffer, EnqueueType type) {
        return impl::Enqueue((char*)buffer.GetPointer(), buffer.GetSize(), type);
    }

    Result ControlService::Remove(u32 index) {
        return impl::Remove(index);
    }

}
