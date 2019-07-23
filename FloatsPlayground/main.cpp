#include <iostream>
#include <cassert>

class Float
{
public:
	Float(char number = 0) : m_Buffer(number) { }

	operator float()
	{
		//TODO
	}

	int sign()
	{
		return m_Buffer & (1u << 7) ? -1 : 1;
	}

	unsigned biased_exponent()
	{
		return (m_Buffer & 0x70u) >> 4;
	}

	int actual_exponent()
	{
		return biased_exponent() - 3;
	}

	float mantissa()
	{
		float mantissa = 1.f;

		if (m_Buffer & (1u << 3))
		{
			mantissa += 0.5f;
		}

		if (m_Buffer & (1u << 2))
		{
			mantissa += 0.25f;
		}

		if (m_Buffer & (1u << 1))
		{
			mantissa += 0.125f;
		}

		if (m_Buffer & (1u << 0))
		{
			mantissa += 0.0625f;
		}

		return mantissa;
	}

private:
	char m_Buffer;
};

void TestSign()
{
	for (int i = 0; i < 128; ++i)
	{
		assert(Float(char(i)).sign() == 1);
	}

	for (int i = 128; i < 256; ++i)
	{
		assert(Float(char(i)).sign() == -1);
	}
}

void TestBiasedExponent()
{
	assert(Float(0x70).biased_exponent() == 7);
	assert(Float(0x60).biased_exponent() == 6);
	assert(Float(0x50).biased_exponent() == 5);
	assert(Float(0x40).biased_exponent() == 4);
	assert(Float(0x30).biased_exponent() == 3);
	assert(Float(0x20).biased_exponent() == 2);
	assert(Float(0x10).biased_exponent() == 1);
	assert(Float(0x00).biased_exponent() == 0);

	assert(Float(0x71).biased_exponent() == 7);
	assert(Float(0x62).biased_exponent() == 6);
	assert(Float(0x53).biased_exponent() == 5);
	assert(Float(0x44).biased_exponent() == 4);
	assert(Float(0x35).biased_exponent() == 3);
	assert(Float(0x26).biased_exponent() == 2);
	assert(Float(0x17).biased_exponent() == 1);
	assert(Float(0x08).biased_exponent() == 0);
	assert(Float(0x79).biased_exponent() == 7);
	assert(Float(0x6A).biased_exponent() == 6);
	assert(Float(0x5B).biased_exponent() == 5);
	assert(Float(0x4C).biased_exponent() == 4);
	assert(Float(0x3D).biased_exponent() == 3);
	assert(Float(0x2E).biased_exponent() == 2);
	assert(Float(0x1F).biased_exponent() == 1);

	assert(Float(0x80).biased_exponent() == 0);
	assert(Float(0x90).biased_exponent() == 1);
	assert(Float(0xA0).biased_exponent() == 2);
	assert(Float(0xB0).biased_exponent() == 3);
	assert(Float(0xC0).biased_exponent() == 4);
	assert(Float(0xD0).biased_exponent() == 5);
	assert(Float(0xE0).biased_exponent() == 6);
	assert(Float(0xF0).biased_exponent() == 7);

	assert(Float(0x81).biased_exponent() == 0);
	assert(Float(0x92).biased_exponent() == 1);
	assert(Float(0xA3).biased_exponent() == 2);
	assert(Float(0xB4).biased_exponent() == 3);
	assert(Float(0xC5).biased_exponent() == 4);
	assert(Float(0xD6).biased_exponent() == 5);
	assert(Float(0xE7).biased_exponent() == 6);
	assert(Float(0xF8).biased_exponent() == 7);
	assert(Float(0x89).biased_exponent() == 0);
	assert(Float(0x9A).biased_exponent() == 1);
	assert(Float(0xAB).biased_exponent() == 2);
	assert(Float(0xBC).biased_exponent() == 3);
	assert(Float(0xCD).biased_exponent() == 4);
	assert(Float(0xDE).biased_exponent() == 5);
	assert(Float(0xEF).biased_exponent() == 6);
}

void TestActualExponent()
{
	assert(Float(0x70).actual_exponent() == 4);
	assert(Float(0x60).actual_exponent() == 3);
	assert(Float(0x50).actual_exponent() == 2);
	assert(Float(0x40).actual_exponent() == 1);
	assert(Float(0x30).actual_exponent() == 0);
	assert(Float(0x20).actual_exponent() == -1);
	assert(Float(0x10).actual_exponent() == -2);
	assert(Float(0x00).actual_exponent() == -3);
}

void TestMantissa()
{
	float epsilon = 0.00001;

	//TODO
	assert((Float(0x08).mantissa() - 1.5f) < epsilon);
	assert((Float(0x07).mantissa() - 2.0f) < epsilon);
	assert((Float(0x06).mantissa() - 1.375f) < epsilon);
	assert((Float(0x05).mantissa() - 1.3125f) < epsilon);
	assert((Float(0x04).mantissa() - 1.25f) < epsilon);
	assert((Float(0x03).mantissa() - 1.1875f) < epsilon);
	assert((Float(0x02).mantissa() - 1.125f) < epsilon);
	assert((Float(0x01).mantissa() - 1.0625f) < epsilon);
	assert((Float(0x00).mantissa() - 1.f) < epsilon);
}

int main()
{
	TestSign();
	TestBiasedExponent();
	TestActualExponent();
	TestMantissa();

	return 0;
}