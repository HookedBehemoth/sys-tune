#include "music_control_service.hpp"

#include "impl/music_player.hpp"

namespace ams::tune {

    Result GetStatus(sf::Out<AudioOutState> out) {
        return impl::GetStatus(out.GetPointer());
    }

    Result Play() {
        return impl::Play();
    }

    Result Pause() {
        return impl::Pause();
    }

    Result Next() {
        impl::Next();
        return ResultSuccess();
    }

    Result Prev() {
        impl::Prev();
        return ResultSuccess();
    }

    Result GetVolume(sf::Out<float> out) {
        return impl::GetVolume(out.GetPointer());
    }

    Result SetVolume(float volume) {
        return impl::SetVolume(volume);
    }

    Result GetRepeatMode(sf::Out<RepeatMode> mode) {
        impl::GetRepeatMode(mode.GetPointer());
        return ResultSuccess();
    }

    Result SetRepeatMode(RepeatMode mode) {
        impl::SetRepeatMode(mode);
        return ResultSuccess();
    }

    Result GetShuffleMode(sf::Out<ShuffleMode> mode) {
        impl::GetShuffleMode(mode.GetPointer());
        return ResultSuccess();
    }

    Result SetShuffleMode(ShuffleMode mode) {
        impl::SetShuffleMode(mode);
        return ResultSuccess();
    }

    Result GetCurrentPlaylistSize(sf::Out<u32> size) {
        impl::GetCurrentPlaylistSize(size.GetPointer());
        return ResultSuccess();
    }

    Result GetCurrentPlaylist(sf::Out<u32> size, sf::OutBuffer buffer) {
        impl::GetCurrentPlaylist(size.GetPointer(), (char*)buffer.GetPointer(), buffer.GetSize());
        return ResultSuccess();
    }

    Result GetCurrentQueueItem(sf::Out<CurrentStats> out, sf::OutBuffer buffer) {
        return impl::GetCurrentQueueItem(out.GetPointer(), (char*)buffer.GetPointer(), buffer.GetSize());
    }

    Result ClearQueue() {
        impl::ClearQueue();
        return ResultSuccess();
    }

    Result MoveQueueItem(u32 src, u32 dst) {
        impl::MoveQueueItem(src, dst);
        return ResultSuccess();
    }

    Result Remove(sf::InBuffer buffer) {
        impl::Remove((char*)buffer.GetPointer(), buffer.GetSize());
        return ResultSuccess();
    }

    Result Enqueue(sf::InBuffer buffer, EnqueueType type) {
        return impl::Enqueue((char*)buffer.GetPointer(), buffer.GetSize(), type);
    }

}
