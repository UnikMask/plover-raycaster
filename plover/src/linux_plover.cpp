#include <plover/plover.h>
#include "linux_plover.h"

#include "Renderer.h"
#include "plover/plover.h"
#include "plover_int.h"
#include "ttfRenderer.h"
#include "instrument.h"

#include <cstddef>
#include <cstdlib>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>

#define LINUX_GAME_SO "../libProjectG.so"
#define GAME_UPDATE_RENDER_FUNC "gameUpdateAndRender"

// OS-Independent Wrappers
// TODO(oliver): Handle errors instead of just asserting
void readFile(const char *path, u8 **buffer, u32 *bufferSize) {
	FILE *fp = fopen(path, "r");
	if (fp != NULL) {
		if (fseek(fp, 0L, SEEK_END) == 0) {
			*bufferSize = ftell(fp);
			assert(*bufferSize != -1);

			*buffer = new u8[sizeof(u8) * (*bufferSize + 1)];
			assert(fseek(fp, 0L, SEEK_SET) == 0);

			*bufferSize = fread(*buffer, sizeof(u8), *bufferSize, fp);
			assert(ferror(fp) == 0);

			(*buffer)[(*bufferSize)++] = '\0';
		}
	}
	fclose(fp);
}

void DEBUG_log(const char *f, ...) {
	va_list args;
	va_start(args, f);
	vprintf(f, args);
	va_end(args);
}

internal_func int linux_guarStub(Handles *_h, GameMemory *_gm) {
	printf("Shared lib not loaded\n");
	return 0;
}

internal_func linux_GameCode linux_loadGameCode(linux_GameCode oldCode) {
	int res;

	struct stat dlStat = {};
	res = stat(LINUX_GAME_SO, &dlStat);
	assert((!res) && "Failed to get shared library attributes!");

	if (dlStat.st_mtim.tv_nsec != oldCode.modificationTime) {
		linux_GameCode newCode = {};
		newCode.modificationTime = dlStat.st_mtim.tv_nsec;
		newCode.updateAndRender = linux_guarStub;

		if (oldCode.sharedObject) {
			res = dlclose(oldCode.sharedObject);
			assert((!res) && "Failed to unload shared object!");
		}
		newCode.sharedObject = dlopen(LINUX_GAME_SO, RTLD_LAZY);
		if (newCode.sharedObject) {
			newCode.updateAndRender = (f_gameUpdateAndRender *)dlsym(
				newCode.sharedObject, GAME_UPDATE_RENDER_FUNC);

			if (!newCode.updateAndRender) {
				newCode.updateAndRender = linux_guarStub;
			}
		}
		return newCode;
	}
	return oldCode;
}

MArena linux_createArena(u64 size) {
	MArena arena{};
	arena.size = size;
	arena.base = calloc(size, sizeof(std::byte));
	arena.head = arena.base;
	return arena;
}

void linux_destroyArena(MArena arena) { free(arena.base); }

// Memory
internal_func GameMemory linux_createMemory() {
	GameMemory memory{};
	memory.initialized = false;
	memory.persistentArena = linux_createArena(Megabytes(64));

	return memory;
}

internal_func void linux_destroyMemory(GameMemory memory) {
	linux_destroyArena(memory.persistentArena);
}

internal_func Handles linux_createHandles() {
	Handles handles{};
	handles.DEBUG_log = DEBUG_log;
	handles.getTime = getTime;
	handles.getInputMessage = getInputMessage;
	handles.pushRenderCommand = pushRenderCommand;
	handles.hasRenderMessage = hasRenderMessage;
	handles.popRenderMessage = popRenderMessage;
	handles.UI_Clear = UI_Clear;
	handles.UI_Rect = UI_Rect;
	handles.UI_Text = UI_Text;
	handles.profileFuncEnter = profilerEnter;
	handles.profileFuncExit = profilerExit;
	return handles;
}

int main() {
	linux_GameCode game{};
	game = linux_loadGameCode(game);
	Handles handles = linux_createHandles();
	GameMemory memory = linux_createMemory();

	spallInit();

	// Initialize font rendering
	if (!ttfInit()) {
		exit(-1);
	}

	ctx.renderer.init();

	bool inTrace = false;
	while (ctx.renderer.render()) {
		if (ctx.profilerRecording && !inTrace) {
			inTrace = true;
			spallInitThread(0, 10 * 1024 * 1024, 10000);
		} else if (!ctx.profilerRecording && inTrace) {
			inTrace = false;
			spallExitThread();
		}

		game = linux_loadGameCode(game);
		game.updateAndRender(&handles, &memory);
		ctx.renderer.processCommands();
	}
	ctx.renderer.cleanup();
	spallQuit();

	linux_destroyMemory(memory);
	return 0;
}
