#pragma once

#include "flappy.h"

#include <vector>
#include <memory>
#include <thread>

class Population
{
public:
	typedef unsigned Fitness;
	typedef unsigned SizeType;
	typedef bool Gene;
	struct Chromosome
	{
		std::vector<Gene> Genes;
		Fitness Fitness;
	};

	Population(const Population& rhs) = delete;
	Population& operator=(const Population& rhs) = delete;

	Population(SizeType populationSize, SizeType chromosomeSize, float selectionRatio, std::shared_ptr<Game>& game);

	bool FoundSolution() const
	{
		return m_Chromosomes[m_Fittest].Fitness == m_Chromosomes[0].Genes.size();
	}

	void NextGeneration();

	Chromosome GetFittest() const
	{
		return m_Chromosomes[m_Fittest];
	}
private:
	void SingleThreadRoutine(std::vector<Chromosome>& newChromosomes);

	void FindFittest();
	void LaunchThreads(std::vector<Chromosome>& newChromosomes, unsigned threadsCount, std::vector<std::thread>& threads);
	void MultiThreadRoutine(std::vector<Chromosome>& newChromosomes, unsigned threadsCount);

	void RandomizeChromosome(Chromosome& chromosome);

	void CalculateFitness();
	Fitness CalculateFitness(const Chromosome& chromosome);

	void Selection(std::vector<Chromosome>& newChromosomes);
	void CrossoverSingleThread(std::vector<Chromosome>& newChromosomes);

	void RandomMutation(Chromosome& mutated);
	void SequentialMutation(Chromosome& mutated);
	void MutationSingleThread(std::vector<Chromosome>& newChromosomes);

	void ThreadCalculateFitness(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end);
	void ThreadMutation(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end);
	void ThreadCrossover(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end);
	void ThreadCrossMutateCalcFitness(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end);

	std::vector<Chromosome> m_Chromosomes;
	SizeType m_ChromosomeSize;
	std::shared_ptr<Game> m_Game;
	SizeType m_Fittest;
	float m_SelectionRatio;
};