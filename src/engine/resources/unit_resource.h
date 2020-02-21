#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "types.h"
#include "vec2.h"
#include "color.h"
#include "transform.h"
#include "rect.h"
#include "resource.h"
#include "resources/sprite_resource.h"

namespace engine
{
enum class AutoDeleteType
{
	None,
	OutOfScreen, // delete when out of screen, any side
	EndOfScreen // delete when exits screen, depending on screen mode vertical/horizontal
};

struct SpriteInstanceResource
{
	struct SpriteResource* sprite = nullptr;
	std::string name;
	std::string animationName = "default";
	Transform transform;
	u32 orderIndex = 0;
	bool collide = true;
	bool visible = true;
	bool shadow = true;
	f32 health = 100.0f;
	f32 maxHealth = 100.0f;
	Color color = Color::black;
	ColorMode colorMode = ColorMode::Add;
	Color hitColor = Color::red;
	std::map<std::string /*anim name*/, struct AnimationResource*> animations;
};

struct WeaponInstanceResource
{
	SpriteInstanceResource* attachTo = nullptr;
	struct WeaponResource* weapon = nullptr;
	Vec2 localPosition;
	f32 ammo = 100;
	bool active = true;
};

struct UnitLifeStage
{
	std::string name;
	f32 triggerOnHealth = 100;
};

struct UnitResource : Resource
{
	enum class Type
	{
		Unknown,
		Player,
		Enemy,
		Item,
		PlayerProjectile,
		EnemyProjectile,

		Count
	};

	std::string name;
	Type type = Type::Unknown;
	f32 speed = 10.0f;
	f32 shadowScale = 1.0f;
	f32 parallaxScale = 1.0f;
	bool shadow = false;
	Vec2 shadowOffset;
	bool visible = true;
	AutoDeleteType autoDeleteType = AutoDeleteType::EndOfScreen;
	bool collide = true;
	std::string rootSpriteInstanceName;
	Json::Value controllersJson;
	struct ScriptResource* script = nullptr;
	std::map<std::string /*sprite instance name*/, SpriteInstanceResource*> spriteInstances;
	std::vector<UnitLifeStage*> stages;
	std::map<std::string /*weapon name*/, WeaponInstanceResource*> weapons;

	virtual bool load(Json::Value& json) override;
};

}
