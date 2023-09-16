/*
* PPA (Player Position Approximation) engines are used for making gameplay look more smooth,
* by inserting fake frames in between the real ones,
* and thus doing things like updating other players 240 times a second,
* despite the server only being 30 ticks per second.
*
* For more information, visit specific header files.
*/

#pragma once

#include "ppa_base.hpp"
#include "ppa_off.hpp"
#include "ppa_dr.hpp"
#include "ppa_interp.hpp"

std::unique_ptr<PPAEngine> pickPPAEngine(int engineId);