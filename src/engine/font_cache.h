#pragma once
#include "types.h"
#include "font.h"

namespace engine
{
class FontCache
{
public:
	FontCache(ImageAtlas* newAtlas);
	~FontCache();
	Font* createFont(const std::string& name, const std::string& filename, u32 size, bool packAtlasNow);
	void releaseFont(Font* font);
	void deleteFonts();
	void rescaleFonts(f32 scale);

protected:
	struct CachedFontInfo
	{
		Font font;
		std::string name;
		std::string filename;
		u32 size;
		u32 usageCount = 0;
	};

	ImageAtlas* atlas = nullptr;
	std::unordered_map<Font*, CachedFontInfo*> cachedFonts;
};

}