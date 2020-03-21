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
            AddToQueue = 3,
            GetNext = 4,
            GetLast = 5,
            Clear = 6,
        };

      public:
        virtual Result GetStatus(sf::Out<PlayerStatus> out);
        virtual Result SetStatus(PlayerStatus status);
        virtual Result GetQueueCount(sf::Out<u32> out);
        virtual Result AddToQueue(sf::InBuffer path);
        virtual Result GetNext(sf::OutBuffer out_path);
        virtual Result GetLast(sf::OutBuffer out_path);
        virtual Result Clear();

      public:
        DEFINE_SERVICE_DISPATCH_TABLE{
            MAKE_SERVICE_COMMAND_META(GetStatus),
            MAKE_SERVICE_COMMAND_META(SetStatus),
            MAKE_SERVICE_COMMAND_META(GetQueueCount),
            MAKE_SERVICE_COMMAND_META(AddToQueue),
            MAKE_SERVICE_COMMAND_META(GetNext),
            MAKE_SERVICE_COMMAND_META(GetLast),
            MAKE_SERVICE_COMMAND_META(Clear),
        };
    };

}
