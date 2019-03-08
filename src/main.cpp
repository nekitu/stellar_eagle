﻿#pragma execution_character_set("utf-8")
#include "engine/types.h"
#include "engine/game.h"

int main(int argc, char** args)
{
	engine::Game game;

	game.windowWidth = 1000;
	game.windowHeight = 800;
	game.windowTitle = "Stellar Eagle";
	game.initialize();
	game.mainLoop();

	return 0;
}
