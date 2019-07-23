#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>

void InitializeRandomGenerator()
{
	static int initialized = 0;

	if (initialized == 0)
	{
		srand((unsigned)time(NULL));

		initialized = 1;
	}
}

void GenerateRandomFloatSequence(float* sequence, size_t length)
{
	InitializeRandomGenerator();

	for (size_t i = 0; i < length; ++i)
	{
		sequence[i] = ((float)rand() / RAND_MAX) * 1000.f;
	}
}

void GenerateIncrementalFloatSequence(float* sequence, size_t length)
{
	float base = 0.f;
	float step = 0.1f;

	for (size_t i = 0; i < length; ++i)
	{
		sequence[i] = base + (i * step);
	}
}

void GenerateEqualFloatSequence(float* sequence, size_t length)
{
	const float VALUE = 0.12345;

	for (size_t i = 0; i < length; ++i)
	{
		sequence[i] = VALUE;
	}
}

void GenerateFloatSequence(float** sequence, size_t length)
{
	*sequence = (float*)malloc(length * sizeof(float));

	if (*sequence == NULL)
	{
		return;
	}

	//GenerateRandomFloatSequence(*sequence, length);
	//GenerateIncrementalFloatSequence(*sequence, length);
	GenerateEqualFloatSequence(*sequence, length);
}

void PrintSequence(const float* sequence, size_t length)
{
	for (size_t i = 0; i < length ; ++i)
	{
		printf("%f\n", sequence[i]);
	}
}

float KahanSummation(const float* sequence, size_t length)
{
	float sum = 0.f;
	float compensator = 0.f;

	for (size_t i = 0; i < length; ++i)
	{
		const float compensated_number = sequence[i] - compensator;
		const float temporary_sum = sum + compensated_number;

		compensator = (temporary_sum - sum) - compensated_number;
		sum = temporary_sum;
	}

	printf("KahanSummation: %f\n", sum);

	return sum;
}

float NaiveSummation(const float* sequence, size_t length)
{
	float sum = 0.f;

	for (size_t i = 0; i < length; ++i)
	{
		sum += sequence[i];
	}

	printf("NaiveSummation: %f\n", sum);

	return sum;
}

int main()
{
	const size_t DEFAULT_SEQUENCE_LENGTH = 500000;

	float* sequence = NULL;

	size_t length = DEFAULT_SEQUENCE_LENGTH;

	GenerateFloatSequence(&sequence, length);
	if (!sequence)
	{
		return 1;
	}

	//PrintSequence(sequence, length);

	KahanSummation(sequence, length);
	NaiveSummation(sequence, length);

	free(sequence);

	return 0;
}