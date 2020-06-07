#pragma once
#include "types.hpp"

namespace ams::tune {

    class ControlService final : public sf::IServiceObject {
      protected:
        /* Command IDs. */
        enum class CommandId {
            GetStatus = 0,
            Play = 1,
            Pause = 2,
            Next = 3,
            Prev = 4,

            GetVolume = 10,
            SetVolume = 11,

            GetRepeatMode = 20,
            SetRepeatMode = 21,
            GetShuffleMode = 22,
            SetShuffleMode = 23,

            GetCurrentPlaylistSize = 30,
            GetCurrentPlaylist = 31,
            GetCurrentQueueItem = 32,
            ClearQueue = 33,
            MoveQueueItem = 34,
            Select = 35,
            Seek = 36,

            Enqueue = 40,
            Remove = 41,

            QuitServer = 50,

            GetApiVersion = 5000,
        };

      public:
        virtual void GetStatus(sf::Out<bool> out);
        virtual void Play();
        virtual void Pause();
        virtual void Next();
        virtual void Prev();

        virtual void GetVolume(sf::Out<float> out);
        virtual void SetVolume(float volume);

        virtual void GetRepeatMode(sf::Out<RepeatMode> mode);
        virtual void SetRepeatMode(RepeatMode mode);
        virtual void GetShuffleMode(sf::Out<ShuffleMode> mode);
        virtual void SetShuffleMode(ShuffleMode mode);

        virtual void GetCurrentPlaylistSize(sf::Out<u32> size);
        virtual void GetCurrentPlaylist(sf::Out<u32> size, sf::OutBuffer buffer);
        virtual Result GetCurrentQueueItem(sf::Out<CurrentStats> out, sf::OutBuffer buffer);
        virtual void ClearQueue();
        virtual void MoveQueueItem(u32 src, u32 dst);
        virtual void Select(u32 index);
        virtual void Seek(u32 position);

        virtual Result Enqueue(sf::InBuffer buffer, EnqueueType type);
        virtual Result Remove(u32 index);

        virtual void QuitServer();

        virtual Result GetApiVersion(sf::Out<u32> version) {
            version.SetValue(TUNE_API_VERSION);
            return ResultSuccess();
        }

      public:
        DEFINE_SERVICE_DISPATCH_TABLE{
            MAKE_SERVICE_COMMAND_META(GetStatus),
            MAKE_SERVICE_COMMAND_META(Play),
            MAKE_SERVICE_COMMAND_META(Pause),
            MAKE_SERVICE_COMMAND_META(Next),
            MAKE_SERVICE_COMMAND_META(Prev),

            MAKE_SERVICE_COMMAND_META(GetVolume),
            MAKE_SERVICE_COMMAND_META(SetVolume),

            MAKE_SERVICE_COMMAND_META(GetRepeatMode),
            MAKE_SERVICE_COMMAND_META(SetRepeatMode),
            MAKE_SERVICE_COMMAND_META(GetShuffleMode),
            MAKE_SERVICE_COMMAND_META(SetShuffleMode),

            MAKE_SERVICE_COMMAND_META(GetCurrentPlaylistSize),
            MAKE_SERVICE_COMMAND_META(GetCurrentPlaylist),
            MAKE_SERVICE_COMMAND_META(GetCurrentQueueItem),
            MAKE_SERVICE_COMMAND_META(ClearQueue),
            MAKE_SERVICE_COMMAND_META(MoveQueueItem),
            MAKE_SERVICE_COMMAND_META(Select),
            MAKE_SERVICE_COMMAND_META(Seek),

            MAKE_SERVICE_COMMAND_META(Enqueue),
            MAKE_SERVICE_COMMAND_META(Remove),

            MAKE_SERVICE_COMMAND_META(QuitServer),

            MAKE_SERVICE_COMMAND_META(GetApiVersion),
        };
    };

}
