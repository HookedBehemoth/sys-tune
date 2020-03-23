#pragma once
#include "types.hpp"

namespace ams::music {

    class ControlService final : public sf::IServiceObject {
      protected:
        /* Command IDs. */
        enum class CommandId {
            GetStatus = 0,
            SetStatus = 1,
            GetVolume = 2,
            SetVolume = 3,
            GetLoop   = 4,
            SetLoop   = 5,

            GetCurrent = 10,
            CountTunes = 11,
            ListTunes = 12,

            AddToQueue = 20,
            Clear = 21,
            Shuffle = 22,
        };

      public:
        virtual Result GetStatus(sf::Out<PlayerStatus> out);
        virtual Result SetStatus(PlayerStatus status);
        virtual Result GetVolume(sf::Out<double> out);
        virtual Result SetVolume(double volume);
        virtual Result GetLoop(sf::Out<LoopStatus> out);
        virtual Result SetLoop(LoopStatus loop);

        virtual Result GetCurrent(sf::OutBuffer out_path, sf::Out<CurrentTune> out_meta);
        virtual Result CountTunes(sf::Out<u32> out);
        virtual Result ListTunes(sf::OutBuffer out_path, sf::Out<u32> out);

        virtual Result AddToQueue(sf::InBuffer path);
        virtual Result Clear();
        virtual Result Shuffle();

      public:
        DEFINE_SERVICE_DISPATCH_TABLE{
            MAKE_SERVICE_COMMAND_META(GetStatus),
            MAKE_SERVICE_COMMAND_META(SetStatus),
            MAKE_SERVICE_COMMAND_META(GetVolume),
            MAKE_SERVICE_COMMAND_META(SetVolume),
            MAKE_SERVICE_COMMAND_META(GetLoop),
            MAKE_SERVICE_COMMAND_META(SetLoop),
            
            MAKE_SERVICE_COMMAND_META(GetCurrent),
            MAKE_SERVICE_COMMAND_META(CountTunes),
            MAKE_SERVICE_COMMAND_META(ListTunes),
          
            MAKE_SERVICE_COMMAND_META(AddToQueue),
            MAKE_SERVICE_COMMAND_META(Clear),
            MAKE_SERVICE_COMMAND_META(Shuffle),
        };
    };

}
