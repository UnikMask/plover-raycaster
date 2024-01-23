#pragma once

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS  // let me use standard functions
#endif

#include <stdint.h>
#include <stddef.h>

// Expose GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef _WIN32
# define EXPORT extern "C" __declspec( dllexport )
#else
# define EXPORT extern "C"
#endif

#define internal_func static
#define global_var static
#define local_persist static

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t  i64;
typedef int32_t  i32;
typedef int16_t  i16;
typedef int8_t   i8;

typedef double   f64;
typedef float    f32;

#define Kilobytes(kb) ((uint64_t) kb * 1024)
#define Megabytes(mb) ((uint64_t) mb * 1024 * 1024)
#define Gigabytes(gb) ((uint64_t) gb * 1024 * 1024 * 1024)

typedef glm::vec2 Vec2;
typedef glm::vec3 Vec3;
typedef glm::vec4 Vec4;
typedef glm::ivec2 IVec2;
typedef glm::ivec3 IVec3;
typedef glm::ivec4 IVec4;
typedef glm::uvec2 UVec2;
typedef glm::uvec3 UVec3;
typedef glm::uvec4 UVec4;
typedef glm::mat3 Mat3;
typedef glm::mat4 Mat4;

#define PI      3.14159265358979
#define HALF_PI 1.57079632679489

const u32 WIDTH = 1280;
const u32 HEIGHT = 720;

#include "render.h"
#include "input.h"

// NOTE(oliver): engine handles
struct Handles {
	void (*DEBUG_log)(const char *, ...);

	// Timing
	double (*getTime)();

	// Input
	InputMessage (*getInputMessage)();

	// Rendering
	void (*pushRenderCommand)(RenderCommand);
	bool (*hasRenderMessage)();
	RenderMessage (*popRenderMessage)();

    // UI
    void (*UI_Clear)();
    void (*UI_Rect)(Vec4, UVec2, UVec2);
	void (*UI_Text)(Vec4, UVec2, const char*, ...);

	// Profiling
	void (*profileFuncEnter)(void *fn, void *caller);
	void (*profileFuncExit)(void *fn, void *caller);
};

struct MArena {
	void *base;
	void *head;
	u64 size;
};

struct GameMemory {
	bool initialized;

	MArena persistentArena;
	u64   _persistentStorageSize;
	void *_persistentStorage;
};

typedef int f_gameUpdateAndRender(Handles*, GameMemory*);
