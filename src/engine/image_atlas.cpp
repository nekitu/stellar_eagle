#include <string.h>
#include "image_atlas.h"
#include "utils.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/stb_image/stb_image_write.c"
#include "texture_array.h"
#include <stb_image.h>
#include <tga.hpp>

namespace engine
{
static u32 atlasId = 0;

ImageAtlas::ImageAtlas(u32 textureWidth, u32 textureHeight)
{
	id = atlasId++;
	create(textureWidth, textureHeight);
	addWhiteImage();
}

ImageAtlas::~ImageAtlas()
{
	for (auto& at : atlasTextures)
	{
		delete[] at->textureImage;
	}

	for (auto& pps : pendingPackImages)
	{
		delete[] pps.imageData;
	}

	delete textureArray;
}

void ImageAtlas::create(u32 textureWidth, u32 textureHeight)
{
	width = textureWidth;
	height = textureHeight;

	for (auto& at : atlasTextures)
	{
		delete [] at->textureImage;
		delete at;
	}
	atlasTextures.clear();
	clearImages();
	delete textureArray;
	textureArray = new TextureArray();
	textureArray->resize(1, textureWidth, textureHeight);
}

AtlasImage* ImageAtlas::getImageById(AtlasImageId id) const
{
	auto iter = images.find(id);

	if (iter == images.end())
	{
		return nullptr;
	}

	return iter->second;
}

AtlasImage* ImageAtlas::addImage(const Rgba32* imageData, u32 width, u32 height, bool addBleedOut)
{
	return addImageInternal(lastImageId++, imageData, width, height, addBleedOut);
}

AtlasImage* ImageAtlas::addImageInternal(AtlasImageId imgId, const Rgba32* imageData, u32 imageWidth, u32 imageHeight, bool addBleedOut)
{
	PackImageData psd;

	u32 imageSize = imageWidth * imageHeight;
	psd.imageData = new Rgba32[imageSize];
	memcpy(psd.imageData, imageData, imageSize * 4);
	psd.id = imgId;
	psd.width = imageWidth;
	psd.height = imageHeight;
	psd.packedRect.set(0, 0, 0, 0);
	psd.atlas = this;
	psd.bleedOut = addBleedOut;
	pendingPackImages.push_back(psd);

	AtlasImage* image = new AtlasImage();

	image->id = psd.id;
	image->width = imageWidth;
	image->height = imageHeight;
	image->atlas = this;
	images.insert(std::make_pair(psd.id, image));

	return image;
}

void ImageAtlas::updateImageData(AtlasImageId imgId, const Rgba32* imageData, u32 width, u32 height)
{
	//TODO
}

void ImageAtlas::deleteImage(AtlasImage* image)
{
	auto iter = images.find(image->id);

	if (iter == images.end())
		return;

	delete[] image->imageData;
	delete image;

	//TODO: we would need to recreate the atlas texture where this image was in
	images.erase(iter);
}

AtlasImage* ImageAtlas::addWhiteImage(u32 width)
{
	u32 whiteImageSize = width * width;
	Rgba32* whiteImageData = new Rgba32[whiteImageSize];

	memset(whiteImageData, 0xff, whiteImageSize * 4);
	whiteImage = addImage(whiteImageData, width, width, true);
	delete[]whiteImageData;

	return whiteImage;
}

bool ImageAtlas::pack(
	u32 spacing,
	const Color& bgColor,
	AtlasPackPolicy packPolicy)
{
	if (pendingPackImages.empty())
		return true;

	lastUsedBgColor = bgColor;
	lastUsedPolicy = packPolicy;
	lastUsedSpacing = spacing;

	u32 border2 = spacing * 2;
	::Rect packedRect;
	bool rotated = false;
	bool allTexturesDirty = false;
	std::vector<PackImageData> acceptedImages;

	while (!pendingPackImages.empty())
	{
		// search some place to put the image
		for (auto& atlasTex : atlasTextures)
		{
			switch (atlasTex->packPolicy)
			{
			case AtlasPackPolicy::Guillotine:
			{
				auto& guillotineBinPack = atlasTex->guillotineBinPack;
				auto iter = pendingPackImages.begin();

				while (iter != pendingPackImages.end())
				{
					auto& packImage = *iter;

					//TODO: if image is bigger than the atlas size, then resize or just skip
					if (packImage.width >= width || packImage.height >= height)
					{
						delete[] packImage.imageData;
						iter = pendingPackImages.erase(iter);
						continue;
					}

					packedRect = guillotineBinPack.Insert(
						packImage.width + border2,
						packImage.height + border2,
						true,
						GuillotineBinPack::FreeRectChoiceHeuristic::RectBestShortSideFit,
						GuillotineBinPack::GuillotineSplitHeuristic::SplitLongerAxis);

					if (packedRect.height <= 0)
					{
						++iter;
						continue;
					}

					packImage.packedRect.x = packedRect.x;
					packImage.packedRect.y = packedRect.y;
					packImage.packedRect.width = packedRect.width;
					packImage.packedRect.height = packedRect.height;
					packImage.atlas = this;
					packImage.atlasTexture = atlasTex;
					acceptedImages.push_back(packImage);
					iter = pendingPackImages.erase(iter);
				}

				break;
			}
			case AtlasPackPolicy::MaxRects:
			{
				MaxRectsBinPack& maxRectsBinPack = atlasTex->maxRectsBinPack;
				auto iter = pendingPackImages.begin();

				while (iter != pendingPackImages.end())
				{
					auto& packImage = *iter;

					packedRect = maxRectsBinPack.Insert(
						packImage.width + border2,
						packImage.height + border2,
						MaxRectsBinPack::FreeRectChoiceHeuristic::RectBestAreaFit);

					if (packedRect.height <= 0)
					{
						++iter;
						continue;
					}

					packImage.packedRect.x = packedRect.x;
					packImage.packedRect.y = packedRect.y;
					packImage.packedRect.width = packedRect.width;
					packImage.packedRect.height = packedRect.height;
					packImage.atlas = this;
					packImage.atlasTexture = atlasTex;
					acceptedImages.push_back(packImage);
					iter = pendingPackImages.erase(iter);
				}

				break;
			}
			case AtlasPackPolicy::ShelfBin:
			{
				ShelfBinPack& shelfBinPack = atlasTex->shelfBinPack;
				auto iter = pendingPackImages.begin();

				while (iter != pendingPackImages.end())
				{
					auto& packImage = *iter;

					packedRect = shelfBinPack.Insert(
						packImage.width + border2,
						packImage.height + border2,
						ShelfBinPack::ShelfChoiceHeuristic::ShelfBestAreaFit);

					if (packedRect.height <= 0)
					{
						++iter;
						continue;
					}

					packImage.packedRect.x = packedRect.x;
					packImage.packedRect.y = packedRect.y;
					packImage.packedRect.width = packedRect.width;
					packImage.packedRect.height = packedRect.height;
					packImage.atlas = this;
					packImage.atlasTexture = atlasTex;
					acceptedImages.push_back(packImage);
					iter = pendingPackImages.erase(iter);
				}

				break;
			}
			case AtlasPackPolicy::Skyline:
			{
				SkylineBinPack& skylineBinPack = atlasTex->skylineBinPack;
				auto iter = pendingPackImages.begin();

				while (iter != pendingPackImages.end())
				{
					auto& packImage = *iter;

					packedRect = skylineBinPack.Insert(
						packImage.width + border2,
						packImage.height + border2,
						SkylineBinPack::LevelChoiceHeuristic::LevelMinWasteFit);

					if (packedRect.height <= 0)
					{
						++iter;
						continue;
					}

					packImage.packedRect.x = packedRect.x;
					packImage.packedRect.y = packedRect.y;
					packImage.packedRect.width = packedRect.width;
					packImage.packedRect.height = packedRect.height;
					packImage.atlas = this;
					packImage.atlasTexture = atlasTex;
					acceptedImages.push_back(packImage);
					iter = pendingPackImages.erase(iter);
				}

				break;
			}
			default:
				break;
			}
		}

		if (!pendingPackImages.empty())
		{
			AtlasTexture* newTexture = new AtlasTexture();

			newTexture->packPolicy = packPolicy;
			newTexture->textureImage = new Rgba32[width * height];
			memset(newTexture->textureImage, 0, width * height * sizeof(Rgba32));
			newTexture->textureIndex = atlasTextures.size();
			newTexture->textureArray = textureArray;

			switch (packPolicy)
			{
			case AtlasPackPolicy::Guillotine:
				newTexture->guillotineBinPack.Init(width, height);
				break;
			case AtlasPackPolicy::MaxRects:
				newTexture->maxRectsBinPack.Init(width, height);
				break;
			case AtlasPackPolicy::ShelfBin:
				newTexture->shelfBinPack.Init(width, height, useWasteMap);
				break;
			case AtlasPackPolicy::Skyline:
				newTexture->skylineBinPack.Init(width, height, useWasteMap);
				break;
			default:
				break;
			}

			atlasTextures.push_back(newTexture);

			// resize the texture array
			textureArray->resize(atlasTextures.size(), width, height);
			allTexturesDirty = true;
		}
	}

	// we have now the rects inside the atlas
	for (auto& packImage : acceptedImages)
	{
		auto image = images[packImage.id];

		assert(image);
		// take out the border from final image rect
		packImage.packedRect.x += spacing;
		packImage.packedRect.y += spacing;
		packImage.packedRect.width -= border2;
		packImage.packedRect.height -= border2;

		// if bleedOut, then limit/shrink the rect so we sample from within the image
		if (packImage.bleedOut)
		{
			const int bleedOutSize = 3;
			packImage.packedRect.x += bleedOutSize;
			packImage.packedRect.y += bleedOutSize;
			packImage.packedRect.width -= bleedOutSize * 2;
			packImage.packedRect.height -= bleedOutSize * 2;
		}

		// init image
		// pass the ownership of the data ptr
		image->bleedOut = packImage.bleedOut;
		image->imageData = packImage.imageData;
		image->width = packImage.width;
		image->height = packImage.height;
		image->atlasTexture = packImage.atlasTexture;
		assert(image->atlasTexture);
		image->rect = { (f32)packImage.packedRect.x, (f32)packImage.packedRect.y, (f32)packImage.width, (f32)packImage.height };
		image->rotated = packImage.width != packImage.packedRect.width;
		image->uvRect.set(
			(f32)packImage.packedRect.x / (f32)width,
			(f32)packImage.packedRect.y / (f32)height,
			(f32)packImage.packedRect.width / (f32)width,
			(f32)packImage.packedRect.height / (f32)height);

		// copy image to the atlas image buffer
		if (image->rotated)
		{
			// rotation is clockwise
			for (u32 y = 0; y < packImage.height; y++)
			{
				for (u32 x = 0; x < packImage.width; x++)
				{
					u32 destIndex =
						packImage.packedRect.x + y + (packImage.packedRect.y + x) * width;
					u32 srcIndex = y * packImage.width + (packImage.width - 1) - x;
					image->atlasTexture->textureImage[destIndex] = ((Rgba32*)packImage.imageData)[srcIndex];
				}
			}
		}
		else
			for (u32 y = 0; y < packImage.height; y++)
			{
				for (u32 x = 0; x < packImage.width; x++)
				{
					u32 destIndex =
						packImage.packedRect.x + x + (packImage.packedRect.y + y) * width;
					u32 srcIndex = x + y * packImage.width;
					image->atlasTexture->textureImage[destIndex] = ((Rgba32*)packImage.imageData)[srcIndex];
				}
			}

		image->atlasTexture->dirty = true;
	}

	for (auto& atlasTex : atlasTextures)
	{
		if (atlasTex->dirty || allTexturesDirty)
		{
			atlasTex->textureArray->updateLayerData(atlasTex->textureIndex, atlasTex->textureImage);
			atlasTex->dirty = false;
		}
	}

	assert(pendingPackImages.empty());

	return pendingPackImages.empty();
}

void ImageAtlas::repackImages()
{
	deletePackerImages();

	for (auto& img : images)
	{
		PackImageData packImg;

		packImg.id = img.first;

		// pass over the data ptr, it will be passed again to the image
		if (img.second->imageData)
			packImg.imageData = img.second->imageData;

		packImg.width = img.second->width;
		packImg.height = img.second->height;
		packImg.packedRect.set(0, 0, 0, 0);
		packImg.atlas = this;
		packImg.bleedOut = img.second->bleedOut;
		pendingPackImages.push_back(packImg);
	}

	packWithLastUsedParams();
}

void ImageAtlas::deletePackerImages()
{
	// initialize the atlas textures
	for (auto& atlasTex : atlasTextures)
	{
		switch (atlasTex->packPolicy)
		{
		case AtlasPackPolicy::Guillotine:
			atlasTex->guillotineBinPack = GuillotineBinPack(width, height);
			break;
		case AtlasPackPolicy::MaxRects:
			atlasTex->maxRectsBinPack = MaxRectsBinPack(width, height);
			break;
		case AtlasPackPolicy::ShelfBin:
			atlasTex->shelfBinPack = ShelfBinPack(width, height, useWasteMap);
			break;
		case AtlasPackPolicy::Skyline:
			atlasTex->skylineBinPack = SkylineBinPack(width, height, useWasteMap);
			break;
		default:
			break;
		}

		// clear texture
		memset(atlasTex->textureImage, lastUsedBgColor.getRgba(), width * height * sizeof(Rgba32));
	}
}

void ImageAtlas::clearImages()
{
	deletePackerImages();

	for (auto& image : images)
	{
		delete [] image.second->imageData;
		delete image.second;
	}

	for (auto& image : pendingPackImages)
	{
		delete [] image.imageData;
	}

	images.clear();
	pendingPackImages.clear();
}


AtlasImage* ImageAtlas::loadImageToAtlas(const std::string& path, PaletteInfo* paletteInfo)
{
	int width = 0;
	int height = 0;
	int comp = 0;
	u8* data = 0;

	if (strstr(path.c_str(), ".tga") || strstr(path.c_str(), ".TGA"))
	{
		tga::TGA tgaFile;

		if (tgaFile.Load(path))
		{
			width = tgaFile.GetWidth();
			height = tgaFile.GetHeight();
			data = tgaFile.GetData();
			auto format = tgaFile.GetFormat();
			auto imgSize = width * height;

			if (paletteInfo)
			{
				paletteInfo->isPaletted = tgaFile.GetIndexedData() != nullptr;
				paletteInfo->bitsPerPixel = tgaFile.GetBpp();
				paletteInfo->colors.resize(tgaFile.GetColorPaletteLength());

				auto pal = tgaFile.GetColorPalette();
				u8* color;

				// copy palette
				for (int i = 0; i < paletteInfo->colors.size(); i++)
				{
					color = &pal[i * 3];

					if (tgaFile.GetFormat() == tga::ImageFormat::RGB)
					{
						paletteInfo->colors[i] =
							packRGBA(color[2], color[1], color[0], 0xff);
					}
					else if (tgaFile.GetFormat() == tga::ImageFormat::RGBA)
					{
						paletteInfo->colors[i] = packRGBA(color[3], color[2], color[1], color[0]);
					}
				}

				// we only support 256 color palettes
				if (paletteInfo->isPaletted && paletteInfo->bitsPerPixel == 8)
				{
					u32* rgbaData = new u32[imgSize];
					u8 index = 0;

					for (int y = 0; y < height; y++)
					{
						for (int x = 0; x < width; x++)
						{
							auto offs = y * width + x;
							index = *(u8*)(tgaFile.GetIndexedData() + offs);
							u32 c = packRGBA(index, 0, 0, 255);
							auto offsFlipped = (height - 1 - y) * width + x;
							rgbaData[offsFlipped] = c;
						}
					}

					data = (u8*)rgbaData;
				}
			}
		}
	}
	else
	{
		data = (u8*)stbi_load(path.c_str(), &width, &height, &comp, 4);
	}

	LOG_INFO("Loaded image: {0} {1}x{2}", path, width, height);

	if (!data)
		return nullptr;

	auto img = addImage((Rgba32*)data, width, height);

	delete[] data;

	return img;
}


}
