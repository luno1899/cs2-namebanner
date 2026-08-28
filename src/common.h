#pragma once
// The SDK uses a few older patterns that modern MSVC warns about.
#ifdef _WIN32
#pragma warning(disable: 4146 4244 4267 4018 4099 4005 5033)
#endif

#include <ISmmPlugin.h>
#include <stdint.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "entity2/entitysystem.h"
#include "tier0/dbg.h"

#define MAXPLAYERS 64

#ifdef _WIN32
#define ROOTBIN "/bin/win64/"
#define GAMEBIN "/csgo/bin/win64/"
#else
#define ROOTBIN "/bin/linuxsteamrt64/"
#define GAMEBIN "/csgo/bin/linuxsteamrt64/"
#endif

PLUGIN_GLOBALVARS();

typedef int8 i8;
typedef int16 i16;
typedef int32 i32;
typedef int64 i64;

typedef uint8 u8;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef float f32;
typedef double f64;

// str*cmp considered harmful.
//  Macros to make sure you don't mess up checking if strings are equal.
//  The I means case insensitive.
#define NB_STREQ(a, b)  (V_strcmp(a, b) == 0)
#define NB_STREQI(a, b) (V_stricmp(a, b) == 0)
// Alias kept so files copied from the upstream CS2AC project compile unmodified.
#define CS2AC_STREQ(a, b) NB_STREQ(a, b)
