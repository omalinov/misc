#include "flappy.h"
#include "Population.h"

#include <cmath>
#include <memory>
#include <iostream>
#include <chrono>

static const Population::SizeType POPULATION_SIZE = 10000;
static const float SELECTION_RATIO = 0.2f;

int main()
{
	auto game = std::make_shared<Game>(FPS,
		HORIZONTAL_VELOCITY,
		VERTICAL_ACCELERATION,
		JUMP_ACCELERATION,
		LevelDescription{ 1000, 100 });

	Population population;

	population.FindSolution(POPULATION_SIZE,
		static_cast<Population::SizeType>(std::floor(game->Level.width / HORIZONTAL_VELOCITY)),
		SELECTION_RATIO,
		game);

	return 0;
}