// PLOVER WIN32 PLATFORM LAYER

#include <plover/plover.h>

#include "Renderer.h"
#include "instrument.h"
#include "glfw.h"

#include "win32_plover.h"
#include "plover_int.h"

void readFile(const char *path, u8 **buffer, u32 *bufferSize) {
	HANDLE hFile = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	*bufferSize = GetFileSize(hFile, (LPDWORD) bufferSize);
	assert(*bufferSize != NULL);

	*buffer = new u8[sizeof(u8) * (*bufferSize + 1)];
	assert(ReadFile(hFile, *buffer, *bufferSize, (LPDWORD) bufferSize, NULL));
	CloseHandle(hFile);
}

void DEBUG_log(const char *f, ...) {
	va_list args;
	va_start(args, f);
	char str[1024];
	vsprintf_s(str, 1024, f, args);
	OutputDebugStringA(str);
	va_end(args);
}

struct win32_GameCode
{
	HMODULE gameCodeDLL;
	FILETIME dllFileTime;
	
	f_gameUpdateAndRender* updateAndRender;
};

internal_func int win32_guarStub(Handles *_h, GameMemory *_gm) {
	OutputDebugStringA("Shared lib not loaded!\n");
	return 0;
}

internal_func win32_GameCode win32_loadGameCode(win32_GameCode gameCode) {
	BOOL res;
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	res = GetFileAttributesExA("../ProjectG.dll", GetFileExInfoStandard, &fileInfo);
	assert(res && "Failed to get DLL attribs!");

	if (CompareFileTime(&fileInfo.ftLastWriteTime, &gameCode.dllFileTime) != 0) {
		win32_GameCode newGameCode = {};
		newGameCode.dllFileTime = fileInfo.ftLastWriteTime;
		newGameCode.updateAndRender = win32_guarStub;

		// NOTE(oliver): Need to unload existing DLL to unlock it
		if (gameCode.gameCodeDLL) {
			res = FreeLibrary(gameCode.gameCodeDLL);
			assert(res && "Failed to unload DLL!");
		}
		res = CopyFileA("../ProjectG.dll", "../ProjectG_tmp.dll", false /* overwrite */);
		assert(res && "Failed to copy DLL!");

		newGameCode.gameCodeDLL = LoadLibraryA("../ProjectG_tmp.dll");
		if (newGameCode.gameCodeDLL) {
			newGameCode.updateAndRender = (f_gameUpdateAndRender*)GetProcAddress(
				newGameCode.gameCodeDLL,
				"gameUpdateAndRender");

			if (!newGameCode.updateAndRender) {
				newGameCode.updateAndRender = win32_guarStub;
			}
		}

		return newGameCode;
	}

	return gameCode;
}

// Memory
MArena win32_createArena(u64 size) {
	MArena arena{};
	arena.size = Megabytes(64);
	arena.base = VirtualAlloc(0,
                              arena.size,
                              MEM_COMMIT | MEM_RESERVE,
                              PAGE_READWRITE); 
	arena.head = arena.base;
	return arena;
}

void win32_destroyArena(MArena arena) {
	VirtualFree(arena.base, 0, MEM_RELEASE);
}

internal_func GameMemory win32_createMemory() {
	GameMemory memory{};
	memory.initialized = false;
	memory.persistentArena = win32_createArena(Megabytes(64));

	return memory;
}

internal_func void win32_destroyMemory(GameMemory& memory) {
	win32_destroyArena(memory.persistentArena);
}

internal_func Handles win32_createHandles() {
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

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
) {
	win32_GameCode game{};
	game = win32_loadGameCode(game);
	Handles handles = win32_createHandles();
	GameMemory memory = win32_createMemory();

	spallInit();

	if (!ttfInit()) {
		exit(-1);
	}

	ctx.renderer.init();

	bool inTrace = false;
	while (ctx.renderer.render()) {
		if (ctx.profilerRecording && !inTrace) {
			inTrace = true;
			spallInitThread(0, Megabytes(10), 10000);
		} else if (!ctx.profilerRecording && inTrace) {
			inTrace = false;
			spallExitThread();
		}

		game = win32_loadGameCode(game);
		game.updateAndRender(&handles, &memory);
		ctx.renderer.processCommands();
	}
	ctx.renderer.cleanup();

	spallQuit();

	win32_destroyMemory(memory);
	return 0;
}
