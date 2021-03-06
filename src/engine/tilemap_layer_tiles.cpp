#include "tilemap_layer_tiles.h"
#include "game.h"
#include "resources/tilemap_resource.h"
#include "resources/tileset_resource.h"
#include "graphics.h"
#include "image_atlas.h"
#include "resource_loader.h"
#include "sprite.h"
#include "utils.h"

namespace engine
{
void TilemapLayerTiles::update(struct Game* game)
{
	Unit::update(game);
}

void TilemapLayerTiles::render(struct Graphics* gfx)
{
	Unit::render(gfx);

	if (!tilemapLayer->visible)
		return;

	switch (tilemapLayer->type)
	{
	case TilemapLayer::Type::Tiles:
		renderTiles(gfx);
		break;
	case TilemapLayer::Type::Image:
		renderImage(gfx);
		break;
	}
}

void TilemapLayerTiles::renderTiles(struct Graphics* gfx)
{
	auto& layer = *tilemapLayer;

	for (auto& chunk : layer.chunks)
	{
		int tileIndex = 0;

		for (auto& tile : chunk.tiles)
		{
			auto& tilesetInfo = layer.tilemapResource->getTilesetInfoByTileId(tile);
			auto tileData = tilesetInfo.tileset->findTileData(tile - tilesetInfo.firstGid);

			if (tileData)
			{
				if (tileData->frames.size())
				{
					tile = tilesetInfo.firstGid + tileData->frames[tileData->currentFrame].tileId;
				}
			}

			if (!tile)
			{
				++tileIndex;
				continue;
			}

			gfx->atlasTextureIndex = tilesetInfo.tileset->image->atlasTexture->textureIndex;
			gfx->color = root->color.getRgba();
			gfx->colorMode = (int)root->colorMode;

			Rect rc;

			f32 col = tileIndex % (u32)chunk.size.x;
			f32 row = tileIndex / (u32)chunk.size.x;

			auto& tileSize = layer.tilemapResource->tileSize;

			auto offsX = tileSize.x * col;
			auto offsY = tileSize.y * row;

			rc.x = boundingBox.x + layer.offset.x + chunk.position.x * tileSize.x + offsX;
			rc.y = boundingBox.y + layer.offset.y + chunk.position.y * tileSize.y + offsY;
			rc.width = layer.tilemapResource->tileSize.x;
			rc.height = layer.tilemapResource->tileSize.y;
			auto uv = tilesetInfo.tileset->getTileRectTexCoord(tile - tilesetInfo.firstGid);

			rc.x = round(rc.x);
			rc.y = round(rc.y);

			gfx->drawQuad(rc, uv, tilesetInfo.tileset->image->rotated);

			++tileIndex;
		}
	}
}

void TilemapLayerTiles::renderImage(Graphics* gfx)
{
	if (!tilemapLayer->repeatCount)
	{
		Rect rc = boundingBox;

		rc.width = tilemapLayer->image->width * imageScale.x;
		rc.height = tilemapLayer->image->height * imageScale.y;
		gfx->drawQuad(rc, tilemapLayer->image->uvRect, tilemapLayer->image->rotated);

		return;
	}

	for (u32 i = 0; i < tilemapLayer->repeatCount; i++)
	{
		Vec2 pos;
		bool imgVisible = false;

		pos.x = boundingBox.x + tilemapLayer->offset.x;
		pos.y = boundingBox.y + tilemapLayer->offset.y;

		if (Game::instance->screenMode == ScreenMode::Vertical)
		{
			pos.y -= i * tilemapLayer->image->height * imageScale.y;

			if (pos.y + tilemapLayer->image->height * imageScale.y < 0)
				break;

			imgVisible = pos.y < gfx->videoHeight;
		}
		else if (Game::instance->screenMode == ScreenMode::Horizontal)
		{
			pos.x -= i * tilemapLayer->image->width * imageScale.x;

			if (pos.x + tilemapLayer->image->width * imageScale.x < 0)
				break;

			imgVisible = pos.x < gfx->videoWidth;
		}

		if (imgVisible)
		{
			Rect rc;

			rc.x = pos.x;
			rc.y = pos.y;
			rc.width = tilemapLayer->image->width * imageScale.x;
			rc.height = tilemapLayer->image->height * imageScale.y;
			gfx->drawQuad(rc, tilemapLayer->image->uvRect, tilemapLayer->image->rotated);
		}
	}
}

}
