#include "config.hpp"
#include "sdmc/sdmc.hpp"
#include "minIni/minIni.h"
#include <cstdio>

namespace config {

namespace {

const char CONFIG_PATH[]{"/config/sys-tune/config.ini"};

void create_config_dir() {
    /* Creating directory on every set call looks sus, but the user may delete the dir */
    /* whilst the sys-mod is running and then any changes made via the overlay */
    /* is lost, which sucks. */
    sdmc::CreateFolder("/config");
    sdmc::CreateFolder("/config/sys-tune");
}

} // namespace

auto get_title_default() -> bool {
    return ini_getbool("config", "default", true, CONFIG_PATH);
}

void set_title_default(bool value) {
    create_config_dir();
    ini_putl("config", "default", value, CONFIG_PATH);
}

auto get_title(u64 tid, bool load_default) -> bool {
    char id_buf[21]{};
    std::sprintf(id_buf, "%016lx", tid);
    const auto default_value = load_default ? get_title_default() : true;
    return ini_getbool("config", id_buf, default_value, CONFIG_PATH);
}

void set_title(u64 tid, bool value) {
    create_config_dir();
    char id_buf[21]{};
    std::sprintf(id_buf, "%016lx", tid);
    ini_putl("config", id_buf, value, CONFIG_PATH);
}

auto get_shuffle() -> bool {
    return ini_getbool("config", "shuffle", false, CONFIG_PATH);
}

void set_shuffle(bool value) {
    create_config_dir();
    ini_putl("config", "shuffle", value, CONFIG_PATH);
}

auto get_repeat() -> int {
    return ini_getl("config", "repeat", 1, CONFIG_PATH);
}

void set_repeat(int value) {
    create_config_dir();
    ini_putl("config", "repeat", value, CONFIG_PATH);
}

auto get_volume() -> int {
    return ini_getl("config", "volume", 1, CONFIG_PATH);
}

void set_volume(int value) {
    create_config_dir();
    ini_putl("config", "volume", value, CONFIG_PATH);
}

} // namespace config
