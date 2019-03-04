#pragma once

#include "Population.h"

#include <random>
#include <algorithm>
#include <cassert>
#include <vector>
#include <iostream>
#include <chrono>

namespace
{
	static const unsigned MIN_MUTATION_SEQUENCE = 2;
	static const unsigned MAX_MUTATION_SEQUENCE = 5;

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

		unsigned Get() const
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

Population::Chromosome Population::FindSolution(SizeType populationSize,
	SizeType chromosomeSize,
	float selectionRatio,
	std::shared_ptr<Game>& game)
{
	m_Chromosomes.resize(populationSize);
	m_ChromosomeSize = chromosomeSize;
	m_Game = game;
	m_Fittest = 0;
	m_SelectionRatio = selectionRatio;

	unsigned allThreads = std::thread::hardware_concurrency();
	if (allThreads == 0)
	{
		SingleThreadRoutine();
	}
	else
	{
		/// Number of cores
		MultiThreadRoutine(allThreads / 2);
	}

	return m_Chromosomes[m_Fittest];
}

void Population::SingleThreadRoutine()
{
	for (SizeType i = 0; i < m_Chromosomes.size(); ++i)
	{
		RandomizeChromosome(m_Chromosomes[i]);
	}

	CalculateFitness();

	long long generation = 1;
	while (!FoundSolution())
	{
		auto start = std::chrono::high_resolution_clock::now();

		std::vector<Chromosome> newChromosomes;

		SelectionSingleThread(newChromosomes);
		CrossoverSingleThread(newChromosomes);
		MutationSingleThread(newChromosomes);

		m_Chromosomes.swap(newChromosomes);

		CalculateFitness();

		auto end = std::chrono::high_resolution_clock::now();
		std::cout << "Generation time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms"
			<< " Fittest: " << GetFittest().Fitness
			<< " generation: " << ++generation << "\n";
	}
}

void Population::FindFittest()
{
	m_Fittest = 0;
	for (SizeType i = 0; i < m_Chromosomes.size(); ++i)
	{
		if (m_Chromosomes[i].Fitness > m_Chromosomes[m_Fittest].Fitness)
		{
			m_Fittest = i;
		}
	}
}

void Population::ThreadInitializeChromosomes(SizeType start, SizeType end)
{
	while (start < end)
	{
		RandomizeChromosome(m_Chromosomes[start]);
		m_Chromosomes[start].Fitness = CalculateFitness(m_Chromosomes[start]);
		++start;
	}
}

void Population::MultiThreadInitializeChromosomes(unsigned threadsCount)
{
	std::vector<std::thread> initializerThreads(threadsCount);

	SizeType chunk = static_cast<SizeType>(std::ceil(static_cast<float>(m_Chromosomes.size()) / threadsCount));

	SizeType chunkStart = 0;
	for (unsigned t = 0; t < threadsCount - 1; ++t)
	{
		SizeType chunkEnd = chunkStart + chunk;
		initializerThreads[t] = std::thread(&Population::ThreadInitializeChromosomes, this, chunkStart, chunkEnd);
		chunkStart = chunkEnd;
	}

	initializerThreads[threadsCount - 1] = std::thread(&Population::ThreadInitializeChromosomes,
		this,
		chunkStart,
		static_cast<SizeType>(m_Chromosomes.size()));

	for (unsigned t = 0; t < threadsCount; ++t)
	{
		initializerThreads[t].join();
	}

	FindFittest();
}

void Population::MultiThreadRoutine(unsigned threadsCount)
{
	MultiThreadInitializeChromosomes(threadsCount);

	if (FoundSolution())
	{
		return;
	}

	std::vector<Chromosome> newChromosomes;
	newChromosomes.resize(m_Chromosomes.size());

	MultiThreadSelection(newChromosomes);

	std::vector<std::thread> threads(threadsCount);
	m_ThreadsWorkingWaitGroup.reset(threadsCount);

	SizeType selected = static_cast<SizeType>(std::floor(m_Chromosomes.size() * m_SelectionRatio));
	/// Skip elites because they will not be changed.
	SizeType chromosomesPerThread = static_cast<SizeType>(std::ceil((m_Chromosomes.size() - selected) / threadsCount));

	SizeType startChunk = selected;
	for (unsigned i = 0; i < threadsCount - 1; ++i)
	{
		SizeType endChunk = startChunk + chromosomesPerThread;
		threads[i] = std::thread(&Population::ThreadRoutine,
			this,
			std::ref(newChromosomes),
			startChunk,
			endChunk);

		startChunk = endChunk;
	}

	/// Last chunk might have a different size.
	threads[threadsCount - 1] = std::thread(&Population::ThreadRoutine,
		this,
		std::ref(newChromosomes),
		startChunk,
		static_cast<SizeType>(m_Chromosomes.size()));

	auto start = std::chrono::high_resolution_clock::now();
	long long generation = 1;
	while (true)
	{
		m_ThreadsWorkingWaitGroup.wait();

		m_Chromosomes.swap(newChromosomes);

		FindFittest();

		if (FoundSolution())
		{
			break;
		}

		auto end = std::chrono::high_resolution_clock::now();
		std::cout << "Generation time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms"
			<< " Fittest: " << GetFittest().Fitness
			<< " generation: " << ++generation << "\n";

		start = std::chrono::high_resolution_clock::now();
		MultiThreadSelection(newChromosomes);

		m_ThreadsWorkingWaitGroup.reset(threadsCount);
		m_ThreadsReadyForWorkWaitGroup.done();
	}

	m_ThreadsReadyForWorkWaitGroup.done();

	for (unsigned i = 0; i < threadsCount; ++i)
	{
		threads[i].join();
	}
}

void Population::ThreadRoutine(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end)
{
	while (!FoundSolution())
	{
		ThreadCrossover(newChromosomes, start, end);
		ThreadMutation(newChromosomes, start, end);
		ThreadCalculateFitness(newChromosomes, start, end);

		std::sort(newChromosomes.begin() + start, newChromosomes.begin() + end, [](const Chromosome& lhs, const Chromosome& rhs) {
			return lhs.Fitness > rhs.Fitness;
		});

		m_ThreadsWorkingWaitGroup.done();
		m_ThreadsWorkingWaitGroup.wait();
		m_ThreadsReadyForWorkWaitGroup.reset(1);
		m_ThreadsReadyForWorkWaitGroup.wait();
	}
}

void Population::MultiThreadSelection(std::vector<Chromosome>& newChromosomes)
{
	std::sort(m_Chromosomes.begin(), m_Chromosomes.end(), [](const Chromosome& lhs, const Chromosome& rhs) {
		return lhs.Fitness > rhs.Fitness;
	});

	SizeType selected = static_cast<SizeType>(std::floor(m_Chromosomes.size() * m_SelectionRatio));

	for (SizeType i = 0; i < selected; ++i)
	{
		newChromosomes[i] = m_Chromosomes[i];
	}
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

void Population::SelectionSingleThread(std::vector<Chromosome>& newChromosomes)
{
	std::sort(m_Chromosomes.begin(), m_Chromosomes.end(), [](const Chromosome& lhs, const Chromosome& rhs) {
		return lhs.Fitness > rhs.Fitness;
	});

	SizeType selected = static_cast<SizeType>(std::floor(m_Chromosomes.size() * m_SelectionRatio));

	for (SizeType i = 0; i < selected; ++i)
	{
		newChromosomes.push_back(m_Chromosomes[i]);
	}
}

namespace
{
	Population::Chromosome DoCrossover(const Population::Chromosome& first, const Population::Chromosome& second)
	{
		assert(first.Genes.size() == second.Genes.size());

		Population::SizeType chromosomeSize = static_cast<Population::SizeType>(first.Genes.size());
		Population::SizeType crossoverPoint = first.Fitness;

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

void Population::CrossoverSingleThread(std::vector<Chromosome>& newChromosomes)
{
	Randomizator randomizatorElites(0, static_cast<unsigned>(newChromosomes.size() - 1));
	Randomizator randomizatorAll(0, static_cast<unsigned>(m_Chromosomes.size() - 1));

	while (newChromosomes.size() < m_Chromosomes.size())
	{
		SizeType firstIndex = randomizatorElites.Get();
		SizeType secondIndex = randomizatorAll.Get();
		while (firstIndex == secondIndex)
		{
			secondIndex = randomizatorAll.Get();
		}

		Chromosome& first = m_Chromosomes[firstIndex];
		Chromosome& second = m_Chromosomes[secondIndex];

		newChromosomes.push_back(DoCrossover(first, second));
		newChromosomes.push_back(DoCrossover(second, first));
	}

	newChromosomes.resize(m_Chromosomes.size());
}

void Population::RandomMutation(Chromosome& mutated)
{
	Randomizator geneRandomizator(0, static_cast<unsigned>(mutated.Genes.size() - 1));

	SizeType mutatedGene = geneRandomizator.Get();
	mutated.Genes[mutatedGene] = !mutated.Genes[mutatedGene];

	mutatedGene = geneRandomizator.Get();
	mutated.Genes[mutatedGene] = !mutated.Genes[mutatedGene];

	mutatedGene = geneRandomizator.Get();
	mutated.Genes[mutatedGene] = !mutated.Genes[mutatedGene];

	mutatedGene = geneRandomizator.Get();
	mutated.Genes[mutatedGene] = !mutated.Genes[mutatedGene];

	mutatedGene = geneRandomizator.Get();
	mutated.Genes[mutatedGene] = !mutated.Genes[mutatedGene];
}

void Population::SequentialMutation(Chromosome& mutated)
{
	Randomizator geneRandomizator(0, static_cast<unsigned>(mutated.Genes.size() - 1));
	Randomizator sequenceRandomizator(MIN_MUTATION_SEQUENCE, MAX_MUTATION_SEQUENCE);

	SizeType sequenceCurrent = geneRandomizator.Get();
	SizeType sequenceEnd = sequenceCurrent + sequenceRandomizator.Get();

	while (sequenceCurrent != sequenceEnd && sequenceCurrent < mutated.Genes.size())
	{
		mutated.Genes[sequenceCurrent] = !mutated.Genes[sequenceCurrent];
		++sequenceCurrent;
	}
}

void Population::MutationSingleThread(std::vector<Chromosome>& newChromosomes)
{
	Coin coin;
	SizeType elites = static_cast<SizeType>(std::floor(m_Chromosomes.size() * m_SelectionRatio)) / 2;

	auto chromosomeIterator = newChromosomes.begin() + elites;
	while (chromosomeIterator != newChromosomes.end())
	{
		/// 12.5%
		if (coin.Flip() == Coin::Face::Head && coin.Flip() == Coin::Face::Head && coin.Flip() == Coin::Face::Head)
		{
			SequentialMutation(*chromosomeIterator);
		}
		/// 25%
		else if (coin.Flip() == Coin::Face::Head && coin.Flip() == Coin::Face::Head)
		{
			RandomMutation(*chromosomeIterator);
		}

		++chromosomeIterator;
	}
}

void Population::ThreadCalculateFitness(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end)
{
	SizeType currentChromosome = start;
	while (currentChromosome < end)
	{
		newChromosomes[currentChromosome].Fitness = CalculateFitness(newChromosomes[currentChromosome]);
		++currentChromosome;
	}
}

void Population::ThreadMutation(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end)
{
	Coin coin;

	SizeType currentChromosome = start;
	while (currentChromosome < end)
	{
		/// 12.5%
		if (coin.Flip() == Coin::Face::Head && coin.Flip() == Coin::Face::Head && coin.Flip() == Coin::Face::Head)
		{
			SequentialMutation(newChromosomes[currentChromosome]);
		}
		/// 25%
		else if (coin.Flip() == Coin::Face::Head && coin.Flip() == Coin::Face::Head)
		{
			RandomMutation(newChromosomes[currentChromosome]);
		}

		++currentChromosome;
	}
}

void Population::ThreadCrossover(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end)
{
	SizeType elites = static_cast<SizeType>(std::floor(m_Chromosomes.size() * m_SelectionRatio));
	Randomizator randomizatorElites(0, elites);
	Randomizator randomizatorAll(0, static_cast<unsigned>(m_Chromosomes.size() - 1));

	SizeType currentChromosome = start;
	while (currentChromosome < end)
	{
		SizeType firstIndex = randomizatorElites.Get();
		SizeType secondIndex = randomizatorAll.Get();
		while (firstIndex == secondIndex)
		{
			secondIndex = randomizatorAll.Get();
		}

		Chromosome& first = m_Chromosomes[firstIndex];
		Chromosome& second = m_Chromosomes[secondIndex];

		newChromosomes[currentChromosome++] = DoCrossover(first, second);
		if (currentChromosome < end)
		{
			newChromosomes[currentChromosome++] = DoCrossover(second, first);
		}
	}
}

