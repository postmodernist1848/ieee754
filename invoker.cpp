#include <cstdint>
#include <cstdio>
#include "ieee754.hpp"

enum TYPE
{
	TYPE_F16,
	TYPE_F32
};

enum OP
{
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV
};
bool parse_type(const char *arg, enum TYPE *t)
{
	if (arg[0] == 'h')
	{
		*t = TYPE_F16;
	}
	else if (arg[0] == 'f')
	{
		*t = TYPE_F32;
	}
	else
	{
		return false;
	}
	if (arg[1] != '\0')
	{
		return false;
	}
	return true;
}

bool parse_op(const char *arg, enum OP *op)
{
	switch (arg[0])
	{
	case '+':
		*op = OP_ADD;
		break;
	case '-':
		*op = OP_SUB;
		break;
	case '*':
		*op = OP_MUL;
		break;
	case '/':
		*op = OP_DIV;
		break;
	default:
		return false;
	}
	if (arg[1] != '\0')
		return false;
	return true;
}



template<typename T, uint32_t EBITS, uint32_t MBITS>
Float<T, EBITS, MBITS> do_op(enum OP op, Float<T, EBITS, MBITS> a, Float<T, EBITS, MBITS> b, enum ROUNDING rounding)
{
    using F = Float<T, EBITS, MBITS>;
	switch (op)
	{
	case OP_ADD:
		return F::add(a, b, rounding);
	case OP_SUB:
		return F::sub(a, b, rounding);
	case OP_MUL:
		return F::mul(a, b, rounding);
	case OP_DIV:
		return F::div(a, b, rounding);
	}
}


template <typename T, uint32_t EBITS, uint32_t MBITS>
Float<T, EBITS, MBITS> perform_test(uint32_t a, uint32_t b, OP op, ROUNDING rounding) {
    using F = Float<T, EBITS, MBITS>;
    F fa = F::from_uint(a), fb = F::from_uint(b);
    F fres = do_op(op, fa, fb, rounding);

    return fres;
}

int main(int argc, char **argv) {
	if (argc < 4) {
        fprintf(stderr, "need more arguments\n");
        return 1;
    }

	enum TYPE type;
    if (!parse_type(argv[1], &type)) {
        fprintf(stderr, "Could not parse type\n");
        return 1;
    }

    uint32_t r;
	sscanf(argv[2], "%u", &r);
	if (r > 3) {
        fprintf(stderr, "Incorrent rounding value `%d`", r);
        return 1;
    }
	enum ROUNDING rounding = (enum ROUNDING)r;

    enum OP op;
    if (!parse_op(argv[3], &op)) {
        fprintf(stderr, "Could not parse operation\n");
        return 1;
    }

    uint32_t a, b, correct_res;
    char exception_bits[3];
    while (true) {
        int n = scanf("%X %X %X %2s", &a, &b, &correct_res, exception_bits);
        if (n == EOF) break;


        uint32_t res;
        if (type == TYPE_F32) {
            Float<uint32_t, F32EBITS, F32MBITS> fres;
            fres = perform_test<uint32_t, F32EBITS, F32MBITS>(a, b, op, rounding); 
            res = fres.s << (F32MBITS + F32EBITS) | fres.e << F32MBITS | fres.m;
#if PRINT_TEST
            printf("%a\n", *((float *)&res));
            print(fres);
#endif
        } else {
            Float<uint16_t, F16EBITS, F16MBITS> fres;
            fres = perform_test<uint16_t, F16EBITS, F16MBITS>(a, b, op, rounding); 
            res = fres.s << (F16MBITS + F16EBITS) | fres.e << F16MBITS | fres.m;
#if PRINT_TEST
            _Float16 t = *((_Float16 *)&res);
            printf("%a\n", (float)t);
            print(fres);
#endif
        }
        int digits = (type == TYPE_F32) ? 8 : 4;
#if PRINT_TEST
        printf("%*x %*x\n", digits, a, digits, b);
#else
        printf("%0*X %0*X %0*X %s\n", digits, a, digits, b, digits, res, exception_bits);
#endif

    }
    return 0;
}
