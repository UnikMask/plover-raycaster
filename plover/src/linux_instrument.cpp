#include <plover/plover.h>

#include "instrument.h"
#include "linux_plover.h"
#include "spall/spall.h"

#include <unistd.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <x86intrin.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <time.h>

static SpallProfile spallCtx;
static _Thread_local SpallBuffer spallBuffer;
static _Thread_local AddrHash addrMap;
static _Thread_local uint32_t tid;
static _Thread_local bool spallThreadRunning = false;

MArena nameArena;

SPALL_FN uint64_t nextPow2(uint64_t x) {
	return 1 << (64 - __builtin_clzll(x - 1));
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
SPALL_FN int ah_hash(void *addr) {
	return (int)(((uint32_t)(uintptr_t)addr) * 2654435769);
}

SPALL_FN void *arenaAllocate_NOINSTRUMENT(MArena *arena, u64 size) {
	assert((u8*) arena->head + size <= (u8*) arena->base + arena->size && "Overflowed arena!");
	void *ptr = arena->head;
	arena->head = (u8*) arena->head + size;

	return ptr;
}

SPALL_FN bool getAddrName(void *addr, AddrName *nameRet) {
	Dl_info info{};
	int status;
	if (dladdr(addr, &info) != 0 && info.dli_sname != NULL) {
		char *realName = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
		if (status == 0) {
			int len = strlen(realName);
			char *heapName = (char *) arenaAllocate_NOINSTRUMENT(&nameArena, len);
			memcpy(heapName, realName, len);
			*nameRet = (AddrName){.str = heapName, .len = len};
		} else {
			*nameRet = (AddrName){.str = (char *) info.dli_sname, .len = (int) strlen(info.dli_sname)};
		}
		return true;
	}

	if (info.dli_fname != NULL) {
		*nameRet = (AddrName){.str = (char *) info.dli_fname, .len = (int) strlen(info.dli_fname)};
		return true;
	}

	return false;
}

SPALL_FN bool ah_get(AddrHash *ah, void *addr, AddrName *nameRet) {
	int addrHash = ah_hash(addr);
	uint64_t hv = ((uint64_t)addrHash) & (ah->hashes.len - 1);
	for (uint64_t i = 0; i < ah->hashes.len; i++) {
		uint64_t idx = (hv + i) & (ah->hashes.len - 1);

		int64_t e_idx = ah->hashes.arr[idx];
		if (e_idx == -1) {

			AddrName name;
			if (!getAddrName(addr, &name)) {
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

		if ((uint64_t)ah->entries.arr[e_idx].addr == (uint64_t)addr) {
			*nameRet = ah->entries.arr[e_idx].name;
			return true;
		}
	}

	// The symbol map is full, make the symbol map bigger!
	return false;
}

SPALL_FN double getRdtscMultiplier(void) {
    unsigned long          tscFreq  = 3000000000;
    bool                   fastPath = false;
    struct perf_event_attr pe        = {
        .type           = PERF_TYPE_HARDWARE,
        .size           = sizeof(struct perf_event_attr),
        .config         = PERF_COUNT_HW_INSTRUCTIONS,
        .disabled       = 1,
        .exclude_kernel = 1,
        .exclude_hv     = 1
    };

    int fd = syscall(298 /* __NR_perf_event_open on x86_64 */, &pe, 0, -1, -1, 0);
    if (fd != -1) {
        void *addr = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
        if (addr) {
            struct perf_event_mmap_page *pc = (perf_event_mmap_page *)addr;
            if (pc->cap_user_time == 1) {
                tscFreq  = ((__uint128_t)1000000000 << pc->time_shift) / pc->time_mult;
                // If you don't like 128 bit arithmetic, do this:
                // tsc_freq  = (1000000000ull << (pc->time_shift / 2)) / (pc->time_mult >> (pc->time_shift - pc->time_shift / 2));
                fastPath = true;
            }
            munmap(addr, 4096);
        }
        close(fd);
    }

    if (!fastPath) {
        // CLOCK_MONOTONIC_RAW is Linux-specific but better;
        // CLOCK_MONOTONIC     is POSIX-portable but slower.
        struct timespec clock = {0};
        clock_gettime(CLOCK_MONOTONIC_RAW, &clock);
        signed long   time_begin = clock.tv_sec * 1e9 + clock.tv_nsec;
        unsigned long tsc_begin  = __rdtsc();
        usleep(2000);
        clock_gettime(CLOCK_MONOTONIC_RAW, &clock);
        signed long   time_end   = clock.tv_sec * 1e9 + clock.tv_nsec;
        unsigned long tsc_end    = __rdtsc();
        tscFreq = (tsc_end - tsc_begin) * 1000000000 / (time_end - time_begin);
    }

    return  1000000.0 / (double) tscFreq;
}

SPALL_NOINSTRUMENT void spallInitThread(uint32_t _tid, size_t buffer_size, int64_t symbol_cache_size) {
	uint8_t *buffer = (uint8_t *)malloc(buffer_size);
	spallBuffer = (SpallBuffer){ .data = buffer, .length = buffer_size };

	// removing initial page-fault bubbles to make the data a little more accurate, at the cost of thread spin-up time
	memset(buffer, 1, buffer_size);

	spall_buffer_init(&spallCtx, &spallBuffer);

	tid = _tid;
	addrMap = ahInit(symbol_cache_size);
	spallThreadRunning = true;
}

SPALL_NOINSTRUMENT void spallExitThread() {
	spallThreadRunning = false;
	ahFree(&addrMap);
	spall_buffer_quit(&spallCtx, &spallBuffer);
	free(spallBuffer.data);
}

SPALL_NOINSTRUMENT void spallInit() {
	nameArena = linux_createArena(Megabytes(64));
	spallCtx = spall_init_file("profile.spall", getRdtscMultiplier());
}

SPALL_NOINSTRUMENT void spallQuit() {
	spall_quit(&spallCtx);
	linux_destroyArena(nameArena);
}

SPALL_FN long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
           int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

char not_found[] = "(unknown name)";
SPALL_NOINSTRUMENT void profilerEnter(void *fn, void *caller) {
	if (!spallThreadRunning) {
		return;
	}

	AddrName name;
	if (!ah_get(&addrMap, fn, &name)) {
		name = (AddrName){.str = not_found, .len = sizeof(not_found) - 1};
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
