#include "flappy.h"
#include "Population.h"

#include <cmath>
#include <memory>
#include <iostream>
#include <chrono>

static const Population::SizeType POPULATION_SIZE = 100;
static const float SELECTION_RATIO = 0.2f;

int main()
{
	auto game = std::make_shared<Game>(FPS,
		HORIZONTAL_VELOCITY,
		VERTICAL_ACCELERATION,
		JUMP_ACCELERATION,
		LevelDescription{ 100, 100 });

	Population population(POPULATION_SIZE,
		static_cast<Population::SizeType>(std::floor(game->Level.width / HORIZONTAL_VELOCITY)),
		SELECTION_RATIO,
		game);

	long long generation = 1;
	while (!population.FoundSolution())
	{
		auto start = std::chrono::high_resolution_clock::now();
		population.NextGeneration();
		auto end = std::chrono::high_resolution_clock::now();

		std::cout << "Generation time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms"
			<< " Fittest: " << population.GetFittest().Fitness
			<< " generation: " << ++generation << "\n";
	}

	return 0;
}