#pragma once

#include <plover/plover.h>
#include "Texture.h"

#include "freetype/ftfntfmt.h"
#include "freetype/ftotval.h"

const u32 MIN_PRINTABLE_CHAR = 32;
const u32 MAX_PRINTABLE_CHAR = 126;

struct VulkanContext;

struct KernData {
	char character;
	i32 kerning_x;
};

struct CharData {
	Bitmap bitmap;
	f32 xAdvance;
	std::vector<KernData> kernData;
};

struct GlyphAtlas {
	CharData chars[MAX_PRINTABLE_CHAR - MIN_PRINTABLE_CHAR + 1]; // NOTE(oliver): printable chars
	ArrayTexture texture;

	u32 charQuadWidth;
	u32 charQuadHeight;
};

bool ttfInit();
void drawGlyphs(u32 x, u32 y, char *text, Bitmap& bitmap);
void buildGlyphAtlas(VulkanContext *context, GlyphAtlas *atlas);
