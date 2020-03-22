#pragma once
#include "types.hpp"

namespace ams::music {

    class ControlService final : public sf::IServiceObject {
      protected:
        /* Command IDs. */
        enum class CommandId {
            GetStatus = 0,
            SetStatus = 1,
            GetQueueCount = 2,
            GetCurrent = 3,
            GetList = 4,
            GetCurrentLength = 5,
            GetCurrentProgress = 6,
            GetVolume = 7,
            SetVolume = 8,
            AddToQueue = 10,
            Clear = 11,
        };

      public:
        virtual Result GetStatus(sf::Out<PlayerStatus> out);
        virtual Result SetStatus(PlayerStatus status);
        virtual Result GetQueueCount(sf::Out<u32> out);
        virtual Result GetCurrent(sf::OutBuffer out_path);
        virtual Result GetList(sf::OutBuffer out_path, sf::Out<u32> out);
        virtual Result GetCurrentLength(sf::Out<double> out);
        virtual Result GetCurrentProgress(sf::Out<double> out);
        virtual Result GetVolume(sf::Out<double> out);
        virtual Result SetVolume(double volume);
        virtual Result AddToQueue(sf::InBuffer path);
        virtual Result Clear();

      public:
        DEFINE_SERVICE_DISPATCH_TABLE{
            MAKE_SERVICE_COMMAND_META(GetStatus),
            MAKE_SERVICE_COMMAND_META(SetStatus),
            MAKE_SERVICE_COMMAND_META(GetQueueCount),
            MAKE_SERVICE_COMMAND_META(GetCurrent),
            MAKE_SERVICE_COMMAND_META(GetList),
            MAKE_SERVICE_COMMAND_META(GetCurrentLength),
            MAKE_SERVICE_COMMAND_META(GetCurrentProgress),
            MAKE_SERVICE_COMMAND_META(GetVolume),
            MAKE_SERVICE_COMMAND_META(SetVolume),
            MAKE_SERVICE_COMMAND_META(AddToQueue),
            MAKE_SERVICE_COMMAND_META(Clear),
        };
    };

}
