#pragma once
#include <string>
#include "types.h"
#include "vec2.h"
#include "resources/sprite_resource.h"
#include "color.h"

namespace engine
{
struct Sprite
{
	std::string name;
	struct SpriteResource* spriteResource = nullptr;
	struct Unit* unit = nullptr;
	Vec2 position;
	Vec2 scale = { 1.0f, 1.0f }; // scale is positive only, use flip to mirror sprite
	bool verticalFlip = false;
	bool horizontalFlip = false;
	f32 rotation = 0;
	Rect localRect;
	Rect rect;
	Rect uvRect;
	Rect shadowRect;
	u32 orderIndex = 0;
	bool visible = true;
	bool collide = true;
	bool shadow = false;
	f32 health = 100;
	f32 maxHealth = 100;
	Color defaultColor = Color::black;
	Color color = Color::black;
	ColorMode colorMode = ColorMode::Add;
	bool relativeToRoot = true;
	std::vector<u32> palette;
	u32 paletteSlot = 0;

	Color hitColor = Color::red;
	ColorMode hitOldColorMode = ColorMode::Add;
	//TODO: could be in SpriteInstanceResource, if you want hit flash customizable per instance and not global same parameters
	f32 hitColorFlashSpeed = 20.0f;
	f32 hitFlashCount = 5;

	bool hitFlashActive = false;
	f32 hitColorTimer = 0.0f;
	f32 currentHitFlashCount = 0;

	SpriteFrameAnimation* frameAnimation = nullptr;
	f32 animationFrame = 0;
	u32 animationRepeatCount = 0;
	f32 animationDirection = 1;
	bool animationIsActive = true;

	void copyFrom(Sprite* other);
	void initializeFrom(struct SpriteInstanceResource* res);
	void update(struct Game* game);
	void setFrameAnimation(const std::string& name);
	void play();
	void hit(f32 hitDamage);
	f32 getFrameFromAngle(f32 angle);
	void setFrameAnimationFromAngle(f32 angle);
	bool checkPixelCollision(Sprite* other, Vec2& outCollisionCenter);
	void copyPaletteFromResource();
	void setPaletteEntry(u32 index, const Color& newColor);
};

}
