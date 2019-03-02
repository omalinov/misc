#pragma once

#include "Population.h"

#include <random>
#include <algorithm>
#include <cassert>

namespace
{
	struct Randomizator
	{
		mutable std::mt19937 generator;
		std::uniform_int_distribution<std::mt19937::result_type> distribution;

		Randomizator(unsigned min, unsigned max)
			: generator()
			, distribution(min, max)
		{
			generator.seed(std::random_device()());
		}

		int Get() const
		{
			return distribution(generator);
		}
	};

	class Coin
	{
	public:
		enum class Face
		{
			Head,
			Tails
		};

		Face Flip() const
		{
			return m_Randomizator.Get() == 0 ? Face::Head : Face::Tails;
		}

		Coin()
			:m_Randomizator(0, 1)
		{
		}
	private:
		Randomizator m_Randomizator;
	};
};

Population::Population(SizeType populationSize, SizeType chromosomeSize, float selectionRatio, std::shared_ptr<Game>& game)
	: m_Chromosomes(populationSize)
	, m_ChromosomeSize(chromosomeSize)
	, m_Game(game)
	, m_Fittest(0)
	, m_SelectionRatio(selectionRatio)
{
	m_Chromosomes.resize(populationSize);
	m_ChromosomeSize = chromosomeSize;

	for (SizeType i = 0; i < populationSize; ++i)
	{
		RandomizeChromosome(m_Chromosomes[i]);
	}

	CalculateFitness();
}

void Population::NextGeneration()
{
	std::vector<Chromosome> newChromosomes;
	newChromosomes.reserve(m_Chromosomes.size());

	Selection(newChromosomes);
	Crossover(newChromosomes);
	Mutation(newChromosomes);

	m_Chromosomes.swap(newChromosomes);

	CalculateFitness();
}

void Population::RandomizeChromosome(Chromosome& chromosome)
{
	static Coin coin;

	chromosome.Genes.resize(m_ChromosomeSize);

	for (SizeType i = 0; i < m_ChromosomeSize; ++i)
	{
		chromosome.Genes[i] = (coin.Flip() == Coin::Face::Head ? true : false);
	}
}

void Population::CalculateFitness()
{
	m_Fittest = 0;

	for (SizeType i = 0; i < m_Chromosomes.size(); ++i)
	{
		m_Chromosomes[i].Fitness = CalculateFitness(m_Chromosomes[i]);

		if (m_Chromosomes[i].Fitness > m_Chromosomes[m_Fittest].Fitness)
		{
			m_Fittest = i;
		}
	}
}

namespace
{
	bool hitsPylon(const Point2d& bird, const LevelDescription::Pylon& pylon)
	{
		const float halfWidth = pylon.width / 2;
		const float halfHeight = pylon.gapHeight / 2;

		const float left = pylon.center.x - halfWidth;
		const float right = pylon.center.x + halfWidth;
		const float up = pylon.center.y - halfHeight;
		const float down = pylon.center.y - halfHeight;

		return (bird.x >= left && bird.x <= right) && (bird.y <= up || bird.y >= down);
	}

	bool isAlive(const Point2d& bird, const LevelDescription& level)
	{
		if (bird.y <= 0 || bird.y >= level.height)
		{
			return false;
		}

		for (const LevelDescription::Pylon& pylon : level.pylons)
		{
			if (hitsPylon(bird, pylon))
			{
				return false;
			}
		}

		return true;
	}
};

/// Returns the number of frames that the bird was alive.
Population::Fitness Population::CalculateFitness(const Chromosome& chromosome)
{
	Population::Fitness fitness = 0;

	Point2d bird{ 0, m_Game->Level.height / 2 };
	Point2d velocity{ m_Game->HorizontalVelocity, 0 };

	for (bool jumpGene : chromosome.Genes)
	{
		/// Always falling, even if jumping.
		velocity.y += m_Game->VerticalAcceleration;
		if (jumpGene)
		{
			velocity.y -= m_Game->JumpAcceleartion;
		}

		bird += velocity;

		if (!isAlive(bird, m_Game->Level))
		{
			return fitness;
		}

		++fitness;
	}

	return fitness;
}

void Population::Selection(std::vector<Chromosome>& newChromosomes)
{
	std::sort(m_Chromosomes.begin(), m_Chromosomes.end(), [](const Chromosome& lhs, const Chromosome& rhs) {
		return lhs.Fitness > rhs.Fitness;
	});

	SizeType selected = static_cast<SizeType>(std::floor(m_Chromosomes.size() * m_SelectionRatio));

	for (SizeType i = 0; i < selected; ++i)
	{
		newChromosomes.push_back(std::move(m_Chromosomes[i]));
	}
}

namespace
{
	Population::Chromosome DoCrossover(const Population::Chromosome& first, const Population::Chromosome& second)
	{
		assert(first.Genes.size() == second.Genes.size());

		Population::SizeType chromosomeSize = static_cast<Population::SizeType>(first.Genes.size());
		Population::SizeType crossoverPoint = chromosomeSize / 2;

		Population::Chromosome child;
		child.Genes.resize(chromosomeSize);

		for (Population::SizeType i = 0; i < crossoverPoint; ++i)
		{
			child.Genes[i] = first.Genes[i];
		}
		for (Population::SizeType i = crossoverPoint; i < chromosomeSize; ++i)
		{
			child.Genes[i] = second.Genes[i];
		}

		return child;
	}
};

void Population::Crossover(std::vector<Chromosome>& newChromosomes)
{
	Randomizator randomizatorSelected(0, static_cast<unsigned>(newChromosomes.size() - 1));
	Randomizator randomizatorRest(static_cast<unsigned>(newChromosomes.size()), static_cast<unsigned>(m_Chromosomes.size() - 1));

	while (newChromosomes.size() < m_Chromosomes.size())
	{
		Chromosome& first = newChromosomes[randomizatorSelected.Get()];
		Chromosome& second = m_Chromosomes[randomizatorRest.Get()];

		newChromosomes.push_back(DoCrossover(first, second));
		newChromosomes.push_back(DoCrossover(second, first));
	}

	newChromosomes.resize(m_Chromosomes.size());
}

void Population::Mutation(std::vector<Chromosome>& newChromosomes)
{
	static Coin coin;

	Randomizator randomizator(0, static_cast<unsigned>(newChromosomes[0].Genes.size() - 1));

	SizeType elites = static_cast<SizeType>(std::floor(m_Chromosomes.size() * m_SelectionRatio)) / 2;

	auto chromosomeIterator = newChromosomes.begin() + elites;
	while (chromosomeIterator != newChromosomes.end())
	{
		if (coin.Flip() == Coin::Face::Head && coin.Flip() == Coin::Face::Head)
		{
			SizeType mutatedGene = randomizator.Get();
			chromosomeIterator->Genes[mutatedGene] = !chromosomeIterator->Genes[mutatedGene];

			mutatedGene = randomizator.Get();
			chromosomeIterator->Genes[mutatedGene] = !chromosomeIterator->Genes[mutatedGene];

			mutatedGene = randomizator.Get();
			chromosomeIterator->Genes[mutatedGene] = !chromosomeIterator->Genes[mutatedGene];

			mutatedGene = randomizator.Get();
			chromosomeIterator->Genes[mutatedGene] = !chromosomeIterator->Genes[mutatedGene];

			mutatedGene = randomizator.Get();
			chromosomeIterator->Genes[mutatedGene] = !chromosomeIterator->Genes[mutatedGene];
		}

		++chromosomeIterator;
	}
}