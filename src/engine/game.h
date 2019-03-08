#pragma once
#include "types.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_version.h>
#include <SDL_audio.h>
#include "unit.h"
#include <string>

namespace engine
{
enum class InputControl
{
	Exit = 0,
	Pause,
	Coin,
	Player1_MoveLeft,
	Player1_MoveRight,
	Player1_MoveUp,
	Player1_MoveDown,
	Player1_Fire1,
	Player1_Fire2,
	Player1_Fire3,
	Player2_MoveLeft,
	Player2_MoveRight,
	Player2_MoveUp,
	Player2_MoveDown,
	Player2_Fire1,
	Player2_Fire2,
	Player2_Fire3,

	Count
};

struct Game
{
	std::string windowTitle = "Game";
	u32 windowWidth = 800, windowHeight = 600;
	bool vSync = true;
	bool exitGame = false;
	SDL_Window* window = nullptr;
	SDL_GLContext glContext = 0;
	struct Graphics* graphics = nullptr;
	struct DataLoader* dataLoader = nullptr;
	f32 deltaTime = 0;
	f32 lastTime = 0;
	std::vector<UnitInstance*> unitInstances;
	UnitInstance* players[2];
	bool controls[(u32)InputControl::Count] = { false };
	std::unordered_map<u32, InputControl> mapSdlToControl;
	struct Music* music = nullptr;
	
	bool initialize();
	void shutdown();
	void createPlayers();
	bool initializeAudio();
	void handleInputEvents();
	void mainLoop();
	void computeDeltaTime();
	bool isControlDown(InputControl control) { return controls[(u32)control]; }
	bool isPlayerMoveLeft(u32 playerIndex);
	bool isPlayerMoveRight(u32 playerIndex);
	bool isPlayerMoveUp(u32 playerIndex);
	bool isPlayerMoveDown(u32 playerIndex);
	bool isPlayerFire1(u32 playerIndex);
	bool isPlayerFire2(u32 playerIndex);
	bool isPlayerFire3(u32 playerIndex);
	struct SpriteInstance* createSpriteInstance(struct Sprite* sprite);
	struct UnitInstance* createUnitInstance(struct Unit* unit);
	struct ProjectileInstance* createProjectileInstance(struct Projectile* projectile);
	struct UnitController* createUnitController(const std::string& name);
};

}