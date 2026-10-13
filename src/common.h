#pragma once
// The SDK uses a few older patterns that modern MSVC warns about.
#ifdef _WIN32
#pragma warning(disable: 4146 4244 4267 4018 4099 4005 5033)
#endif

#include <ISmmPlugin.h>
#include <stdint.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>

#include "const.h"
#include "vector.h"
#include "entity2/entitysystem.h"
#include "tier0/dbg.h"

#define MAXPLAYERS 64

PLUGIN_GLOBALVARS();
