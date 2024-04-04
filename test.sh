set -e

PRINT_TEST=$1
[ -z "$PRINT_TEST" ] && PRINT_TEST=0

clang++ -Wall -Wextra -DPRINT_TEST=$PRINT_TEST invoker.cpp ieee754.cpp -o invoker

runTests() {
    case "$2" in
        0)
            rounding="-rminMag"
            ;;
        1)
            rounding="-rnear_even"
            ;;
        2)
            rounding="-rmax"
            ;;
        3)
            rounding="-rmin"
            ;;
        *)
            echo "test.sh: Unknown rounding"
            exit 1
    esac

    case "$1" in
        *sub)
            op="-"
            ;;
        *add)
            op="+"
            ;;
        *mul)
            op="*"
            ;;
        *div)
            op="/"
            ;;
    esac

    case "$1" in
        f32*)
            t="f"
            ;;
        f16*)
            t="h"
            ;;
    esac

    if [ $PRINT_TEST == "0" ]; then
        ./testfloat_gen "$1" "$rounding" | ./invoker "$t" "$2" "$op" | ./testfloat_ver "$1" "$rounding" -errors 20
    else
        ./testfloat_gen "$1" "$rounding" | ./invoker "$t" "$2" "$op" | python ver.py
    fi
}

# 0towards zero:         "-rminMag"
# 1nearest even:         "-rnear_even"
# 2towards negative inf: "-rmin"
# 3towards positive inf: "-rmax"

runTests "f32_add" 0
runTests "f32_add" 1
runTests "f32_add" 2
runTests "f32_add" 3

runTests "f32_sub" 0
runTests "f32_sub" 1
runTests "f32_sub" 2
runTests "f32_sub" 3

runTests "f32_mul" 1
runTests "f32_mul" 0
runTests "f32_mul" 2
runTests "f32_mul" 3

runTests "f32_div" 0
runTests "f32_div" 1
runTests "f32_div" 2
runTests "f32_div" 3

runTests "f16_add" 0
runTests "f16_add" 1
runTests "f16_add" 2
runTests "f16_add" 3

runTests "f16_sub" 0
runTests "f16_sub" 1
runTests "f16_sub" 2
runTests "f16_sub" 3

runTests "f16_mul" 0
runTests "f16_mul" 1
runTests "f16_mul" 2
runTests "f16_mul" 3

runTests "f16_div" 0
runTests "f16_div" 1
runTests "f16_div" 2
runTests "f16_div" 3
