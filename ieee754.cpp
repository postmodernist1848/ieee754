#include <cstdint>
#include <cstdio>
#include "ieee754.hpp"

void round(uint64_t *resm, enum ROUNDING rounding, bool first_rounded_bit, bool sticky_bit, uint32_t sign)
{
	switch (rounding)
	{
	case ROUNDING_TOWARD_ZERO:
		break;
	case ROUNDING_TOWARD_NEAREST_EVEN:
		if (first_rounded_bit && !sticky_bit)
		{
			// to even case
			if (*resm & 1)
				*resm += 1;
		}
		else if (first_rounded_bit && sticky_bit)
		{
			// up case
			*resm += 1;
		}
		// down case = do nothing
		break;
	case ROUNDING_TOWARD_POS_INFINITY:
		if (!sign && (first_rounded_bit || sticky_bit))
		{
			*resm += 1;
		}
		break;
	case ROUNDING_TOWARD_NEG_INFINITY:
		if (sign && (first_rounded_bit || sticky_bit))
		{
			*resm += 1;
		}
		break;
	}
}

void print(Float32 x)
{
	if (x.is_nan())
	{
		puts("nan");
		return;
	}
	if (x.s)
	{
		putchar('-');
	}
	if (x.is_inf())
	{
		puts("inf");
	}
	else if (x.is_zero())
	{
        puts("0x0.000000p+0");
	}
	else
	{
		uint32_t m = x.m;
		int32_t e = x.e;

		NORM(e, m, F32MBITS);

		// 1. remove (mbits + 1)th bit
		m &= ~((uint32_t)1 << (F32MBITS));

		// 2. shift to align with nibbles
		uint32_t digits;

        m <<= 1;
        digits = 6;
		printf("0x1.%0*xp%+d\n", digits, m, e - BIAS(F32EBITS));
	}
}

void print(Float16 x)
{
	if (x.is_nan())
	{
		puts("nan");
		return;
	}
	if (x.s)
	{
		putchar('-');
	}
	if (x.is_inf())
	{
		puts("inf");
	}
	else if (x.is_zero())
	{
        puts("0x0.000p+0");
	}
	else
	{
		uint32_t m = x.m;
		int32_t e = x.e;

		NORM(e, m, F16MBITS);

		// 1. remove (mbits + 1)th bit
		m &= ~((uint32_t)1 << (F16MBITS));

		// 2. shift to align with nibbles
		uint32_t digits;

        m <<= 2;
        digits = 3;
		printf("0x1.%0*xp%+d\n", digits, m, e - BIAS(F16EBITS));
	}
}

