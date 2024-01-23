#pragma once

#include <plover/plover.h>

#include <spall/spall.h>

struct AddrName {
	char *str;
	int len;
};

struct SymEntry {
	void *addr;
	AddrName name;
};

struct SymArr {
	SymEntry *arr;
	u64 len;
	u64 cap;
};

struct HashArr {
	i64 *arr;
	u64 len;
};

struct AddrHash{
	SymArr  entries;	
	HashArr hashes;	
};

SPALL_NOINSTRUMENT void profilerEnter(void *fn, void *caller);
SPALL_NOINSTRUMENT void profilerExit(void *fn, void *caller);
SPALL_NOINSTRUMENT void spallInit();
SPALL_NOINSTRUMENT void spallQuit();
SPALL_NOINSTRUMENT void spallInitThread(u32 _tid, u64 bufferSize, i64 symbolCacheSize);
SPALL_NOINSTRUMENT void spallExitThread();
