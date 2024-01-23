#pragma once

#include <plover/plover.h>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#include <fileapi.h>
#include <intrin.h>
#include <dbghelp.h>

MArena win32_createArena(u64 size);
void win32_destroyArena(MArena arena);
GameMemory win32_createMemory();
