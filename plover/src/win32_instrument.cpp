#include "instrument.h"
#include "win32_plover.h"
#include "plover_int.h"

static SpallProfile spallCtx;
static _Thread_local SpallBuffer spallBuffer;
static _Thread_local AddrHash addr_map;
static _Thread_local u32 tid;
static _Thread_local bool spallThreadRunning = false;

HANDLE hProcess;
MArena nameArena;

SPALL_FN void *arenaAllocate_NOINSTRUMENT(MArena *arena, u64 size) {
	assert((u8*) arena->head + size <= (u8*) arena->base + arena->size && "Overflowed arena!");
	void *ptr = arena->head;
	arena->head = (u8*) arena->head + size;

	return ptr;
}

SPALL_FN bool win32_initSymbolHandler() {
	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

	hProcess = GetCurrentProcess();

	if (!SymInitialize(hProcess, NULL, TRUE))
	{
		// SymInitialize failed
		DWORD error = GetLastError();
		printf("Failed to initialize symbol handler: %lu\n", error);
		return false;
	}
	return true;
}

SPALL_FN bool win32_getAddrName(void *addr, AddrName *name) {
	DWORD64  dwDisplacement = 0;
	DWORD64  dwAddress = (DWORD64) addr;

	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;

	if (SymFromAddr(hProcess, dwAddress, &dwDisplacement, pSymbol))
	{
	    char *nameStr = (char *) arenaAllocate_NOINSTRUMENT(&nameArena, pSymbol->NameLen);

		memcpy(nameStr, pSymbol->Name, pSymbol->NameLen);
		*name = {
			.str = nameStr,
			.len = (int) pSymbol->NameLen
		};
		return true;
	}
	else
	{
		DWORD error = GetLastError();
		printf("Failed to get symbol from address: %lu\n", error);
		return false;
	}
}

SPALL_FN f64 win32_getRdtscMultiplier()
{
    unsigned long long tscFreq  = 3000000000;
    bool fastPath = false;
    HMODULE ntdll = LoadLibrary("ntdll.dll");
    if (ntdll)
    {
        int (*NtQuerySystemInformation)(int, void *, unsigned int, unsigned int *) =
            (int (*)(int, void *, unsigned int, unsigned int *))GetProcAddress(ntdll, "NtQuerySystemInformation");
        if (NtQuerySystemInformation)
        {
			DEBUG_log("TEST");
            volatile unsigned long long *HSUV   = 0;
            unsigned int size   = 0;
            int result = NtQuerySystemInformation(0xc5, (void **)&HSUV, sizeof(HSUV), &size);
            if (size == sizeof(HSUV) && result >= 0)
            {
                tscFreq  = (10000000ull << 32) / (HSUV[1] >> 32);
                fastPath = true;
            }
        }
        FreeLibrary(ntdll);
    }

    if (!fastPath)
    {
        LARGE_INTEGER frequency; QueryPerformanceFrequency(&frequency);
        LARGE_INTEGER qpc_begin; QueryPerformanceCounter(&qpc_begin);
        unsigned long long tsc_begin = __rdtsc();
        Sleep(2);
        LARGE_INTEGER qpc_end; QueryPerformanceCounter(&qpc_end);
        unsigned long long tsc_end   = __rdtsc();
        tscFreq = (tsc_end - tsc_begin) * frequency.QuadPart / (qpc_end.QuadPart - qpc_begin.QuadPart);
    }

    return 1000000.0 / (double) tscFreq;
}

SPALL_FN f64 nextPow2(u64 x) {
	return 1 << (64 - __lzcnt64(x - 1));;
}

// This is not thread-safe... Use one per thread!
SPALL_FN AddrHash ahInit(int64_t size) {
	AddrHash ah;

	ah.entries.cap = size;
	ah.entries.arr = (SymEntry *) calloc(sizeof(SymEntry), size);
	ah.entries.len = 0;

	ah.hashes.len = nextPow2(size);
	ah.hashes.arr = (i64 *) malloc(sizeof(int64_t) * ah.hashes.len);

	for (int64_t i = 0; i < ah.hashes.len; i++) {
		ah.hashes.arr[i] = -1;
	}

	return ah;
}

SPALL_FN void ahFree(AddrHash *ah) {
	free(ah->entries.arr);
	free(ah->hashes.arr);
	memset(ah, 0, sizeof(AddrHash));
}

// fibhash addresses
SPALL_FN int ahHash(void *addr) {
	return (int)(((u32)(uintptr_t)addr) * 2654435769);
}

SPALL_FN bool ahGet(AddrHash *ah, void *addr, AddrName *nameRet) {
	int addrHash = ahHash(addr);
	u64 hv = ((u64)addrHash) & (ah->hashes.len - 1);
	for (u64 i = 0; i < ah->hashes.len; i++) {
		u64 idx = (hv + i) & (ah->hashes.len - 1);

		i64 e_idx = ah->hashes.arr[idx];
		if (e_idx == -1) {

			AddrName name;
			if (!win32_getAddrName(addr, &name)) {
				// Failed to get a name for the address!
				return false;
			}

			SymEntry entry = {.addr = addr, .name = name};
			ah->hashes.arr[idx] = ah->entries.len;
			ah->entries.arr[ah->entries.len] = entry;
			ah->entries.len += 1;

			*nameRet = name;
			return true;
		}

		if ((u64)ah->entries.arr[e_idx].addr == (u64)addr) {
			*nameRet = ah->entries.arr[e_idx].name;
			return true;
		}
	}

	// The symbol map is full, make the symbol map bigger!
	return false;
}

SPALL_NOINSTRUMENT void spallInitThread(u32 _tid,
                                        u64 bufferSize,
                                        i64 symbolCacheSize) {
	u8 *buffer = (u8 *)malloc(bufferSize);
	spallBuffer = (SpallBuffer){ .data = buffer, .length = bufferSize };

	// removing initial page-fault bubbles to make the data a little more accurate, at the cost of thread spin-up time
	memset(buffer, 1, bufferSize);

	spall_buffer_init(&spallCtx, &spallBuffer);

	tid = _tid;
	addr_map = ahInit(symbolCacheSize);
	spallThreadRunning = true;
}

SPALL_NOINSTRUMENT void spallExitThread() {
	spallThreadRunning = false;
	ahFree(&addr_map);
	spall_buffer_quit(&spallCtx, &spallBuffer);
	free(spallBuffer.data);
}

SPALL_NOINSTRUMENT void spallInit() {
	if (!win32_initSymbolHandler()) {
		exit(-1);
	}

	nameArena = win32_createArena(Megabytes(64));
	spallCtx = spall_init_file("profile.spall", win32_getRdtscMultiplier());
}

SPALL_NOINSTRUMENT void spallQuit() {
	spall_quit(&spallCtx);
	win32_destroyArena(nameArena);
}

char notFound[] = "(unknown name)";
SPALL_NOINSTRUMENT void profilerEnter(void *fn, void *caller) {
	if (!spallThreadRunning) {
		return;
	}

	AddrName name;
	if (!ahGet(&addr_map, fn, &name)) {
		name = (AddrName){.str = notFound, .len = sizeof(notFound) - 1};
	}

	spall_buffer_begin_ex(&spallCtx, &spallBuffer, name.str, name.len, __rdtsc(), tid, 0);
}

SPALL_NOINSTRUMENT void profilerExit(void *fn, void *caller) {
	if (!spallThreadRunning) {
		return;
	}

	spall_buffer_end_ex(&spallCtx, &spallBuffer, __rdtsc(), tid, 0);
}

EXPORT SPALL_NOINSTRUMENT void __cyg_profile_func_enter(void *fn, void *caller) {
    profilerEnter(fn, caller);
}

EXPORT SPALL_NOINSTRUMENT void __cyg_profile_func_exit(void *fn, void *caller) {
    profilerExit(fn, caller);
}
