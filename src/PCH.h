#pragma once

#include "OBSE/OBSE.h"
#include "RE/Oblivion.h"
#include "UE/Unreal.h"

#include "REL/Trampoline.h"

#include "REX/W32/OLE32.h"
#include "REX/W32/SHELL32.h"

#include <unordered_set>
#include <set>

#include "glaze/glaze.hpp"

#include <ClibUtil/rng.hpp>
#include <ClibUtil/simpleINI.hpp>
#include <ClibUtil/string.hpp>

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif

#include <xbyak/xbyak.h>

namespace ini = clib_util::ini;
namespace string = clib_util::string;

using namespace std::literals;

#include "Version.h"
