#include "menu_layer.hpp"
#include "play_layer.hpp"
#include "pause_layer.hpp"
#include "gj_base_game_layer.hpp"
#include "level_info_layer.hpp"
#include "profile_page.hpp"

//TODO: do this but for mac
#ifdef GEODE_IS_WINDOWS
#include "cceglview.hpp"
#endif

#ifndef GEODE_IS_MACOS
#include "loading_layer.hpp"
#endif