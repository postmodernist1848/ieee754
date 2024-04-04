#include "ieee754.hpp"

int main() {
    Float32 a = Float32::from_uint(0x42F7147B); // 123.54
    Float32 b = Float32::from_uint(0x3C4985F0); // 0.0123
    Float32 res = Float32::add(a, b, ROUNDING_TOWARD_NEAREST_EVEN);
    print(res);
}
