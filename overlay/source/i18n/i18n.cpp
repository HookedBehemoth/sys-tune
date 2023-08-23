#include "i18n.hpp"

#include <switch.h>

namespace i18n {

    namespace {

        constexpr i18n::TranslationArray German = i18n::build_translation({{
            #include "german.hpp"
        }});

        constexpr i18n::TranslationArray English = i18n::build_translation({{
            #include "english.hpp"
        }});

        void init_impl(SetLanguage language) {
            switch (language) {
                case SetLanguage_DE:
                    s_translation = &German;
                    break;
                case SetLanguage_ENUS:
                case SetLanguage_ENGB:
                default:
                    s_translation = &English;
                    break;
            }
        }

    }

    const TranslationArray *s_translation;

    void init() {
        Result rc = setInitialize();

        if (R_SUCCEEDED(rc)) {
            u64 language = 0;
            SetLanguage set_language = {};
            if (R_SUCCEEDED(setGetSystemLanguage(&language)) && R_SUCCEEDED(setMakeLanguage(language, &set_language))) {
                init_impl(set_language);
            }
            setExit();
        }
    }
}
