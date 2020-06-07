#include "music_control_service.hpp"

#include "impl/music_player.hpp"

namespace ams::tune {

    extern ams::sf::hipc::ServerManager<1> g_server_manager;

    void ControlService::GetStatus(sf::Out<bool> out) {
        out.SetValue(impl::GetStatus());
    }

    void ControlService::Play() {
        impl::Play();
    }

    void ControlService::Pause() {
        impl::Pause();
    }

    void ControlService::Next() {
        impl::Next();
    }

    void ControlService::Prev() {
        impl::Prev();
    }

    void ControlService::GetVolume(sf::Out<float> out) {
        out.SetValue(impl::GetVolume());
    }

    void ControlService::SetVolume(float volume) {
        impl::SetVolume(volume);
    }

    void ControlService::GetRepeatMode(sf::Out<RepeatMode> mode) {
        mode.SetValue(impl::GetRepeatMode());
    }

    void ControlService::SetRepeatMode(RepeatMode mode) {
        impl::SetRepeatMode(mode);
    }

    void ControlService::GetShuffleMode(sf::Out<ShuffleMode> mode) {
        mode.SetValue(impl::GetShuffleMode());
    }

    void ControlService::SetShuffleMode(ShuffleMode mode) {
        impl::SetShuffleMode(mode);
    }

    void ControlService::GetCurrentPlaylistSize(sf::Out<u32> size) {
        size.SetValue(impl::GetCurrentPlaylistSize());
    }

    void ControlService::GetCurrentPlaylist(sf::Out<u32> size, sf::OutBuffer buffer) {
        size.SetValue(impl::GetCurrentPlaylist((char*)buffer.GetPointer(), buffer.GetSize()));
    }

    Result ControlService::GetCurrentQueueItem(sf::Out<CurrentStats> out, sf::OutBuffer buffer) {
        return impl::GetCurrentQueueItem(out.GetPointer(), (char*)buffer.GetPointer(), buffer.GetSize());
    }

    void ControlService::ClearQueue() {
        impl::ClearQueue();
    }

    void ControlService::MoveQueueItem(u32 src, u32 dst) {
        impl::MoveQueueItem(src, dst);
    }

    void ControlService::Select(u32 index) {
        impl::Select(index);
    }

    void ControlService::Seek(u32 position) {
        impl::Seek(position);
    }

    Result ControlService::Enqueue(sf::InBuffer buffer, EnqueueType type) {
        return impl::Enqueue((char*)buffer.GetPointer(), buffer.GetSize(), type);
    }

    Result ControlService::Remove(u32 index) {
        return impl::Remove(index);
    }

    void ControlService::QuitServer() {
        g_server_manager.RequestStopProcessing();
    }

}
