#pragma once

#include <cstddef>
#include <string_view>
#include <array>
#include <tuple>
#include <cassert>
#include <initializer_list>

namespace i18n {

    namespace {

        using TranslationRawElement = std::tuple<std::string_view, std::string_view>;
        using TranslationInitializerList = std::initializer_list<TranslationRawElement>;

        constexpr
        size_t TranslationCount = TranslationInitializerList {
            #include "english.hpp"
        }.size();

        using RawTranslation = std::array<TranslationRawElement, TranslationCount>;
        using TranslationArray = std::array<std::string_view, TranslationCount>;

        consteval
        TranslationArray build_translation_indices(const RawTranslation &raw) {
            TranslationArray out;
            size_t idx = 0;
            for (auto &[key, value] : raw) {
                out[idx++] = key;
            }
            return out;
        }

        constexpr
        TranslationArray TranslationLookupStrings = build_translation_indices({{
            #include "english.hpp"
        }});

        consteval
        int translation_index_of(std::string_view str) {
            size_t idx = 0;
            while (str != TranslationLookupStrings[idx]) {
                ++idx;
                assert(idx != TranslationLookupStrings.size());
            }
            return idx;
        }

        consteval
        TranslationArray build_translation(const RawTranslation &raw) {
            TranslationArray out;
            for (auto &[key, value] : raw) {
                auto idx = translation_index_of(key);
                out[idx] = value;
            }
            return out;
        }

    }

    extern const TranslationArray *s_translation;

    struct Lang {
        unsigned int index;
        
        std::string_view operator()() {
            return (*s_translation)[index];
        }
    };

    void init();

}

consteval
i18n::Lang operator ""_lang(const char* str, size_t len) {
    return i18n::Lang {
        (unsigned)i18n::translation_index_of({ str, len })
    };
}

