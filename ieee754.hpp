#pragma once

#include <cstdint>

enum ROUNDING
{
	ROUNDING_TOWARD_ZERO = 0,
	ROUNDING_TOWARD_NEAREST_EVEN = 1,
	ROUNDING_TOWARD_POS_INFINITY = 2,
	ROUNDING_TOWARD_NEG_INFINITY = 3
};

#define F32EBITS 8
#define F32MBITS 23

#define F16EBITS 5
#define F16MBITS 10

#define BITS_MAX_VALUE(x) ((1u << x) - 1)
#define BIAS(ebits) ((1 << (ebits - 1)) - 1)
#define QUIETBIT(mbits) (1u << (mbits - 1))

void round(uint64_t *resm, enum ROUNDING rounding, bool first_rounded_bit, bool sticky_bit, uint32_t sign);

// using bitfields, so that sizeof(Float32) == 4 in most (if not all) implementations
template<typename T, uint32_t EBITS, uint32_t MBITS>
struct Float {
	T s : 1;
	T e : EBITS;
	T m : MBITS;

    bool is_inf() { return e == BITS_MAX_VALUE(EBITS) && m == 0; };
    bool is_nan() { return e == BITS_MAX_VALUE(EBITS) && m != 0; };
    bool is_zero() { return e == 0 && m == 0; };
    bool is_subnormal() { return e == 0 && m != 0; };

    static Float<T, EBITS, MBITS> add(Float<T, EBITS, MBITS> a, Float<T, EBITS, MBITS> b, ROUNDING rounding);
    static Float<T, EBITS, MBITS> sub(Float<T, EBITS, MBITS> a, Float<T, EBITS, MBITS> b, ROUNDING rounding);
    static Float<T, EBITS, MBITS> mul(Float<T, EBITS, MBITS> a, Float<T, EBITS, MBITS> b, ROUNDING rounding);
    static Float<T, EBITS, MBITS> div(Float<T, EBITS, MBITS> a, Float<T, EBITS, MBITS> b, ROUNDING rounding);

    static const Float NAN;
    static Float from_uint(T x);
private:
    static Float construct_with_over_under_flow(T sign, int32_t res_exp, uint64_t resm, ROUNDING rounding);
};

typedef Float<uint32_t, F32EBITS, F32MBITS> Float32;
typedef Float<uint16_t, F16EBITS, F16MBITS> Float16;

void print(Float32 x);
void print(Float16 x);

template <typename T, uint32_t EBITS, uint32_t MBITS> 
const Float<T, EBITS, MBITS> Float<T, EBITS, MBITS>::NAN = Float<T, EBITS, MBITS> { 0, BITS_MAX_VALUE(EBITS), QUIETBIT(MBITS) };

template <typename T, uint32_t EBITS, uint32_t MBITS>
Float<T, EBITS, MBITS> Float<T, EBITS, MBITS>::from_uint(T x) {
    return Float<T, EBITS, MBITS> { 
        .s = static_cast<T>((x >> (EBITS + MBITS)) & 1),
        .e = static_cast<T>((x >> MBITS) & BITS_MAX_VALUE(EBITS)),
        .m = static_cast<T>(x & BITS_MAX_VALUE(MBITS)) 
    };
}

template <typename T, uint32_t EBITS, uint32_t MBITS> 
Float<T, EBITS, MBITS> Float<T, EBITS, MBITS>::construct_with_over_under_flow(T sign, int32_t res_exp, uint64_t resm, enum ROUNDING rounding)
{
    using F = Float<T, EBITS, MBITS>;
    if (res_exp >= static_cast<int32_t>(BITS_MAX_VALUE(EBITS)))
    {
        // overflow
        switch (rounding)
        {
        case ROUNDING_TOWARD_NEAREST_EVEN:
            return F { sign, BITS_MAX_VALUE(EBITS), 0 };
        case ROUNDING_TOWARD_ZERO:
            return F { sign, BITS_MAX_VALUE(EBITS) - 1, BITS_MAX_VALUE(MBITS) };
        case ROUNDING_TOWARD_NEG_INFINITY:
            return sign ? F { sign, BITS_MAX_VALUE(EBITS), 0 } : F { sign, BITS_MAX_VALUE(EBITS) - 1, BITS_MAX_VALUE(MBITS) };
        case ROUNDING_TOWARD_POS_INFINITY:
            return sign ? F { sign, BITS_MAX_VALUE(EBITS) - 1, BITS_MAX_VALUE(MBITS) } : F { sign, BITS_MAX_VALUE(EBITS), 0 };
        }
    }
    // exp <= -127
    if (res_exp <= 0)
    {
        // how much we need to shift to represent as subnormal (or zero)
        // -126 is the subnormal exponent
        uint32_t exp_diff = 1 - res_exp;	// >= 1

        bool first_rounded_bit;
        bool sticky_bit;
        // we shift more than the size of integer type
        if (exp_diff >= 64)
        {
            first_rounded_bit = 0;
            sticky_bit = resm != 0;
            resm = 0;
        }
        else
        {
            // get exp_diff'th bit
            first_rounded_bit = (resm & (1llu << (exp_diff - 1))) != 0;
            // last (exp_diff - 1) bits
            sticky_bit = (resm & ((1llu << (exp_diff - 1)) - 1)) != 0;
            resm >>= exp_diff;
        }
        round(&resm, rounding, first_rounded_bit, sticky_bit, sign);
        res_exp = 0;

        // if rounding resulted in 2.0
        if (resm >> MBITS)
        {
            resm <<= 1;
            res_exp++;
        }
    }

    return F { sign, static_cast<T>(res_exp), static_cast<T>(resm & BITS_MAX_VALUE(MBITS)) };
}

// normalize mantissa m if subnormal
// while also modifying exponent e accordingly
#define NORM(e, m, mbits)                                                                                                  \
    do { \
        if (e == 0 && m != 0)                                                                                              \
        {                                                                                                                  \
            e = 1;                                                                                                         \
            while ((m >> mbits) != 1)                                                                                      \
            {                                                                                                              \
                m <<= 1;                                                                                                   \
                e--;                                                                                                       \
            }                                                                                                              \
        } \
    } while (0)

template<typename T, uint32_t EBITS, uint32_t MBITS>
Float<T, EBITS, MBITS> Float<T, EBITS, MBITS>::add(Float<T, EBITS, MBITS> a, Float<T, EBITS, MBITS> b, enum ROUNDING rounding)
{
    using F = Float<T, EBITS, MBITS>;
	if (a.is_nan())
	{
		a.m |= QUIETBIT(MBITS);
		return a;
	}
	if (b.is_nan())
	{
		b.m |= QUIETBIT(MBITS);
		return b;
	}

	if (a.is_inf() && b.is_inf())
	{
		if (a.s != b.s)
			return F::NAN;
		else
			return a;
	}
	if (a.is_inf())
		return a;
	if (b.is_inf())
		return b;

	if (b.e > a.e || (b.e == a.e && b.m > a.m))
	{
		Float t = b;
		b = a;
		a = t;
	}
	// abs(a) >= abs(b)
	// so we only check b for zero
	if (b.is_zero())
	{
		if (rounding == ROUNDING_TOWARD_NEG_INFINITY && a.is_zero() && a.s != b.s)
		{
			return F { 1, 0, 0 };
		}
		if (a.is_zero() && a.s && !b.s)
			return b;
		return a;
	}

	uint64_t am = a.m;
	uint64_t bm = b.m;
	int32_t ae = a.e;
	int32_t be = b.e;

	NORM(ae, am, MBITS);
	NORM(be, bm, MBITS);

	am |= (1 << MBITS);
	bm |= (1 << MBITS);

	if ((uint32_t)(ae - be) > MBITS + 2)
	{
		int32_t res_exp = ae;
		uint64_t resm = am;
		if (a.s == b.s)
		{
			// first_rounded_bit = 0
			// sticky_bit = (b is not 0) which is true
			round(&resm, rounding, 0, 1, a.s);
		}
		else
		{
			resm -= 1;	  // borrowing for subtraction of small b
						  // Ex: case of ae - be == 25:
						  //
						  //            this '1' is borrowed
						  //             \/
						  // [a bits...1010000000                   ]
						  // [b bits             01011000           ]
						  //                         ^ also, imagine this is the last '1' bit of b so it borrows
						  // [      ...100111111110101              ]
						  //                     ^   ^
						  //                     1   2
						  //          1) first_rounded_bit == 1 // all '0' bits after last '1' become '1'
						  //          2) sticky_bit == 1
						  //

			// 0b100000000000000000000000 * 2**26 - 0b1
			// 10000000000000000000000000000000000000000000000000
			//                                                  1
			// 0.1111111111111111111111111111111111111111111111111
			if (!(resm >> MBITS))
			{
				resm <<= 1;
				resm |= 1;
				res_exp--;
			}
			round(&resm, rounding, 1, 1, a.s);
		}
		// if rounding resulted in resm == 2.0
		if (resm >> (MBITS + 1))
		{
			resm >>= 1;
			res_exp++;
		}
		return F::construct_with_over_under_flow(a.s, res_exp, resm, rounding);
	}

	// bm = 1.(23 bits)
	if (ae > be)
	{
		// am = 1.(23 bits) << (ae - be)
		am <<= (ae - be);
		ae = be;
	}
	uint64_t resm;
	if (a.s == b.s)
	{
		// bm = 1.(23 bits)
		// am = 1.(23 bits) << (ae - be)
		// am + bm = (ae - be + 1bits).(23bits)
		resm = am + bm;
	}
	else
	{
		resm = am - bm;	   // am >= bm
	}

	if (resm == 0)
	{
		if (rounding == ROUNDING_TOWARD_NEG_INFINITY)
			return F { 1, 0, 0 };
		return F { 0, 0, 0 };
	}
	// normalize while preserving rounding information
	bool first_rounded_bit = 0;
	bool sticky_bit = 0;
	int32_t res_exp = ae;
	while (resm >> (MBITS + 1))
	// >= 2.0
	{
		sticky_bit |= first_rounded_bit;
		first_rounded_bit = resm & 1;
		resm >>= 1;
		res_exp++;
	}
	// < 1.0
	while (!(resm >> MBITS))
	{
		resm <<= 1;
		res_exp--;
	}

	round(&resm, rounding, first_rounded_bit, sticky_bit, a.s);

	// if rounding resulted in resm == 2.0
	if (resm >> (MBITS + 1))
	{
		resm >>= 1;
		res_exp++;
	}

	return F::construct_with_over_under_flow(a.s, res_exp, resm, rounding);
}

template<typename T, uint32_t EBITS, uint32_t MBITS>
Float<T, EBITS, MBITS> Float<T, EBITS, MBITS>::sub(Float<T, EBITS, MBITS> a, Float<T, EBITS, MBITS> b, enum ROUNDING rounding)
{
	// negate b
	return add(a, Float<T, EBITS, MBITS>{ static_cast<T>(b.s ^ 1), b.e, b.m }, rounding);
}

template<typename T, uint32_t EBITS, uint32_t MBITS>
Float<T, EBITS, MBITS> Float<T, EBITS, MBITS>::mul(Float<T, EBITS, MBITS> a, Float<T, EBITS, MBITS> b, enum ROUNDING rounding)
{
    using F = Float<T, EBITS, MBITS>;
	if (a.is_nan())
	{
		a.m |= QUIETBIT(MBITS);
		return a;
	}
	if (b.is_nan())
	{
		b.m |= QUIETBIT(MBITS);
		return b;
	}

	T sign = a.s ^ b.s;

	if (a.is_zero() || b.is_zero())
	{
		if (a.is_inf() || b.is_inf())
		{
			return F::NAN;
		}
		else
		{
			return F { sign, 0, 0 };
		}
	}

	if (a.is_inf() || b.is_inf())
	{
		return F { sign, BITS_MAX_VALUE(EBITS), 0 };
	}

	uint64_t am = a.m;
	uint64_t bm = b.m;
	int32_t ae = a.e;
	int32_t be = b.e;

	NORM(ae, am, MBITS);
	NORM(be, bm, MBITS);

	int32_t res_exp = ae + be - BIAS(EBITS);

	// add implicit 1 and perform integer multiplication
	uint64_t resm = (am | (1 << MBITS)) * (bm | (1 << MBITS));
	bool first_rounded_bit = (resm >> (MBITS - 1)) & 1;
	bool sticky_bit = (resm & ((1 << (MBITS - 1)) - 1)) != 0;
	resm >>= MBITS;

	// 1.(23 bits) * 1.(23 bits) >> 23 = (2bits).(23 bits)
	if (resm >> (MBITS + 1))
	{
		sticky_bit |= first_rounded_bit;
		first_rounded_bit = resm & 1;
		resm >>= 1;
		res_exp++;
	}

	// 1.(23 bits) *
	// 1.(23 bits) = (2bits).(46 bits)
	if (res_exp > 0)
	{
		round(&resm, rounding, first_rounded_bit, sticky_bit, sign);
		// if rounding resulted in resm == 2.0
		if (resm >> (MBITS + 1))
		{
			resm >>= 1;
			res_exp++;
		}
	}
	else
	{
		resm <<= 1;
		res_exp--;
		sticky_bit |= first_rounded_bit;
		resm |= sticky_bit;	   // save sticky bit as last bit which is going to be shifted anyway
	}
	return F::construct_with_over_under_flow(sign, res_exp, resm, rounding);
}

template<typename T, uint32_t EBITS, uint32_t MBITS>
Float<T, EBITS, MBITS> Float<T, EBITS, MBITS>::div(Float<T, EBITS, MBITS> a, Float<T, EBITS, MBITS> b, enum ROUNDING rounding)
{
    using F = Float<T, EBITS, MBITS>;
	if (a.is_nan())
	{
		a.m |= QUIETBIT(MBITS);
		return a;
	}
	if (b.is_nan())
	{
		b.m |= QUIETBIT(MBITS);
		return b;
	}
	if ((a.is_zero() && b.is_zero()) || (a.is_inf() && b.is_inf()))
	{
		return F::NAN;
	}

	T sign = a.s ^ b.s;

	if (b.is_zero() || a.is_inf())
	{
		return F{ sign, BITS_MAX_VALUE(EBITS), 0 };
	}

	if (a.is_zero() || b.is_inf())
	{
		return F{ sign, 0, 0 };
	}

	uint64_t am = a.m;
	uint64_t bm = b.m;
	int32_t ae = a.e;
	int32_t be = b.e;

	NORM(ae, am, MBITS);
	NORM(be, bm, MBITS);
	int32_t res_exp = ae - be + BIAS(EBITS);

	am |= (1 << MBITS);
	bm |= (1 << MBITS);

	if (am < bm)
	{
		am <<= 1;
		res_exp--;
	}
	// am >= bm

	uint64_t dividend = am << (MBITS + 1);
	uint64_t divisor = bm;

	// (24-25 bits).(24 '0' bits) / 1.(23 bits)
	// if am >= bm then result is 1.(24 bits)
	uint64_t resm = dividend / divisor;
	uint64_t rem = dividend % divisor;

	bool first_rounded_bit = resm & 1;
	bool sticky_bit = rem != 0;
	resm >>= 1;

	if (res_exp > 0)
	{
		round(&resm, rounding, first_rounded_bit, sticky_bit, sign);
		// if rounding resulted in resm == 2.0
		if (resm >> (MBITS + 1))
		{
			resm >>= 1;
			res_exp++;
		}
	}
	else
	{
		resm <<= 1; // shift left to make space for sticky bit
		res_exp--;
		sticky_bit |= first_rounded_bit;
		resm |= sticky_bit;	   // save sticky bit as last bit which is going to be shifted anyway
	}

	return F::construct_with_over_under_flow(sign, res_exp, resm, rounding);
}

