#pragma once

#include "flappy.h"
#include "WaitGroup.hpp"

#include <vector>
#include <memory>
#include <thread>
#include <condition_variable>

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

	Population()
	{
	}

	Chromosome FindSolution(SizeType populationSize, SizeType chromosomeSize, float selectionRatio, std::shared_ptr<Game>& game);
private:
	bool FoundSolution() const
	{
		return m_Chromosomes[m_Fittest].Fitness == m_Chromosomes[0].Genes.size();
	}

	Chromosome GetFittest() const
	{
		return m_Chromosomes[m_Fittest];
	}

	void SingleThreadRoutine();

	void FindFittest();

	void ThreadInitializeChromosomes(SizeType start, SizeType end);
	void MultiThreadInitializeChromosomes(unsigned threadsCount);
	void MultiThreadRoutine(unsigned threadsCount);

	void RandomizeChromosome(Chromosome& chromosome);

	void CalculateFitness();
	Fitness CalculateFitness(const Chromosome& chromosome);

	void SelectionSingleThread(std::vector<Chromosome>& newChromosomes);
	void CrossoverSingleThread(std::vector<Chromosome>& newChromosomes);

	void RandomMutation(Chromosome& mutated);
	void SequentialMutation(Chromosome& mutated);
	void MutationSingleThread(std::vector<Chromosome>& newChromosomes);

	void ThreadCalculateFitness(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end);
	void ThreadMutation(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end);
	void ThreadCrossover(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end);
	void ThreadRoutine(std::vector<Chromosome>& newChromosomes, SizeType start, SizeType end);

	void MultiThreadSelection(std::vector<Chromosome>& newChromosomes);

	std::vector<Chromosome> m_Chromosomes;
	SizeType m_ChromosomeSize;
	std::shared_ptr<Game> m_Game;
	SizeType m_Fittest;
	float m_SelectionRatio;
	WaitGroup m_ThreadsReadyForWorkWaitGroup;
	WaitGroup m_ThreadsWorkingWaitGroup;
};