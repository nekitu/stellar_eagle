#include "tilemap_resource.h"
#include "game.h"
#include "resource_loader.h"
#include <stb_image.h>
#include "image_atlas.h"
#include "graphics.h"
#include "json/writer.h"

namespace engine
{
std::string jsonAsString(const Json::Value& json)
{
	std::string result;
	Json::StreamWriterBuilder wbuilder;

	wbuilder["indentation"] = ""; // Optional
	result = Json::writeString(wbuilder, json);
	return result;
}

void TilemapObject::load(Json::Value& json)
{
	gid = json.get("gid", gid).asUInt();
	id = json.get("id", id).asUInt();
	size.x = json.get("width", size.x).asFloat();
	size.y = json.get("height", size.y).asFloat();
	position.x = json.get("x", position.x).asFloat();
	position.y = json.get("y", position.y).asFloat();
	rotation = json.get("rotation", rotation).asFloat();
	name = json.get("name", name).asString();
	typeString = json.get("type", typeString).asString();
	templatePath = json.get("template", templatePath).asString();

	if (json.get("ellipse", false).asBool()) type = Type::Ellipse;
	else if (json.get("point", false).asBool()) type = Type::Point;
	else if (json.isMember("polygon")) type = Type::Polygon;
	else if (json.isMember("polyline")) type = Type::Polyline;
	else if (json.isMember("gid")) type = Type::Tile;
	else if (json.isMember("text")) type = Type::Text;
	else type = Type::Rect;

	if (type == Type::Polygon || type == Type::Polyline)
	{
		auto& ptsJson = json.get("points", Json::ValueType::arrayValue);

		for (auto& ptJson : ptsJson)
		{
			Vec2 pt;

			pt.x = ptJson.get("x", 0).asFloat();
			pt.y = ptJson.get("y", 0).asFloat();
			points.push_back(pt);
		}
	}
	else if (type == Type::Text)
	{
		auto& textJson = json.get("text", Json::ValueType::objectValue);
		text = textJson.get("text", text).asString();
		wrap = textJson.get("wrap", wrap).asBool();
	}

	auto& propertiesJson = json.get("properties", Json::ValueType::arrayValue);

	LOG_INFO("Properties:");
	for (auto& propJson : propertiesJson)
	{
		auto str = jsonAsString(propJson.get("value", Json::Value()));
		properties[propJson.get("name", "").asString()] = str;
		LOG_INFO("\t{0} = {1}", propJson.get("name", "").asCString(), str.c_str());
	}
}

void TilemapLayer::load(Json::Value& json)
{
	id = json.get("id", 0).asUInt();
	name = json.get("name", "").asString();
	size.x = json.get("width", 0).asInt();
	size.y = json.get("height", 0).asInt();
	start.x = json.get("startx", 0).asInt();
	start.y = json.get("starty", 0).asInt();
	visible = json.get("visible", true).asBool();
	position.x = json.get("x", 0).asInt();
	position.y = json.get("y", 0).asInt();
	opacity = json.get("opacity", 1).asFloat();

	auto& propertiesJson = json.get("properties", Json::ValueType::arrayValue);

	LOG_INFO("Properties:");
	for (auto& propJson : propertiesJson)
	{
		if (propJson.get("name", "").asString() == "cameraParallax")
		{
			cameraParallax = propJson.get("value", 0).asFloat();
		}
		else if (propJson.get("name", "").asString() == "cameraParallaxScale")
		{
			cameraParallaxScale = propJson.get("value", 0).asFloat();
		}
		else if (propJson.get("name", "").asString() == "cameraScroll")
		{
			cameraScroll = propJson.get("value", 0).asFloat();
		}

		auto str = jsonAsString(propJson.get("value", Json::Value()));
			properties[propJson.get("name", "").asString()] = str;
			LOG_INFO("\t{0} = {1}", propJson.get("name", "").asCString(), str.c_str());
	}

	auto typeName = json.get("type", "").asString();

	if (typeName == "tilelayer") type = TilemapLayer::Type::Tiles;

	if (typeName == "group")
	{
		type = TilemapLayer::Type::Group;

		auto& layersJson = json.get("layers", Json::ValueType::arrayValue);

		for (auto& layerJson : layersJson)
		{
			TilemapLayer layer;

			layer.tilemapResource = tilemapResource;
			layer.load(layerJson);
			layers.push_back(layer);
		}
	}

	if (typeName == "imagelayer")
	{
		type = TilemapLayer::Type::Image;
		imagePath = json.get("image", "").asString();
		imagePath = getParentPath(tilemapResource->path) + imagePath;
		replaceAll(imagePath, "\\/", "/");

		int imgWidth = 0;
		int imgHeight = 0;
		int comp = 0;

		stbi_uc* imgData = stbi_load(imagePath.c_str(), &imgWidth, &imgHeight, &comp, 4);
		LOG_INFO("Loaded layer image: {0} {1}x{2}", imagePath, imgWidth, imgHeight);

		if (imgData)
		{
			image = Game::instance->graphics->atlas->addImage((Rgba32*)imgData, imgWidth, imgHeight);
		}
	}

	if (typeName == "objectgroup")
	{
		type = TilemapLayer::Type::Objects;
		auto& objectsJson = json.get("objects", Json::ValueType::arrayValue);

		for (auto& objJson : objectsJson)
		{
			TilemapObject obj;
			std::string tplPath = objJson.get("template", "").asString();

			if (tplPath != "")
			{
				Json::Value tplJson;

				if (loadJson(tplPath, tplJson))
				{
					obj.load(tplJson);
				}
			}

			obj.load(objJson);
			objects.push_back(obj);
		}
	}

	auto& chunksJson = json.get("chunks", Json::ValueType::arrayValue);
	auto& dataJson = json.get("data", Json::ValueType::arrayValue);

	bool infinite = dataJson.isNull();

	if (infinite)
	{
		for (auto& chunkJson : chunksJson)
		{
			TilemapChunk chunk;

			chunk.size.x = chunkJson.get("width", 0).asInt();
			chunk.size.y = chunkJson.get("height", 0).asInt();
			chunk.position.x = chunkJson.get("x", 0).asInt();
			chunk.position.y = chunkJson.get("y", 0).asInt();

			auto& tiles = chunkJson.get("data", Json::ValueType::arrayValue);

			for (auto& tileIndex : tiles)
			{
				chunk.tiles.push_back(tileIndex.asInt());
			}

			chunks.push_back(chunk);
		}
	}
	else
	{
		TilemapChunk chunk;

		chunk.size = size;
		chunk.position = position;

		for (auto& tileIndex : dataJson)
		{
			chunk.tiles.push_back(tileIndex.asInt());
		}

		chunks.push_back(chunk);
	}
}

bool TilemapResource::load(Json::Value& json)
{
	// loading Tiled json map
	tileSize.x = json.get("tilewidth", 0).asInt();
	tileSize.y = json.get("tileheight", 0).asInt();
	infinite = json.get("infinite", true).asBool();

	auto& layersArray = json.get("layers", Json::ValueType::arrayValue);

	for (auto& layerJson : layersArray)
	{
		TilemapLayer layer;

		layer.tilemapResource = this;
		layer.load(layerJson);
		layers.push_back(layer);
	}

	auto& tilesetsArray = json.get("tilesets", Json::ValueType::arrayValue);

	for (auto& tilesetJson : tilesetsArray)
	{
		TilesetInfo info;
		auto srcPath = tilesetJson.get("source", "").asString();

		replaceAll(srcPath, "..", "");
		replaceAll(srcPath, "\/", "/");
		replaceAll(srcPath, ".json", "");

		info.tileset = Game::instance->resourceLoader->loadTileset(srcPath);
		info.firstGid = tilesetJson.get("firstgid", 0).asInt();
		tilesets.push_back(info);
	}

	return true;
}

void TilemapResource::unload()
{
	layers.clear();
}

TilesetInfo TilemapResource::getTilesetInfoByTileId(u32 tileId)
{
	for (int i = 0; i < tilesets.size(); i++)
	{
		if (i + 1 <= tilesets.size() - 1)
		{
			if (tileId >= tilesets[i].firstGid
				&& tileId < tilesets[i + 1].firstGid)
			{
				return tilesets[i];
			}
		}
		else
		{
			return tilesets[i];
		}
	}

	return TilesetInfo();
}

}
