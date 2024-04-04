# IEEE754 Software implementation

An attempt at a readable implementation of IEEE754 floating poing operations. 

Example use (see [example.cpp](example.cpp)):
```console
g++ example.cpp ieee754.cpp -o example
./example
```

## Features
- Float32 and Float16 types
- Addition, subtraction, multiplication and division operations
- Float<>::from_uint() - Float type from IEEE754 format representation
- Print floats (similarly to %a format specifier in printf)

### Tests
To test operations with [Berkeley TestFloat](http://www.jhauser.us/arithmetic/TestFloat.html):
```
./test.sh 0 # required executables: testfloat_gen, testfloat_ver
```
To test print():
```
./test.sh 1 # ver.py compares against printf's %a output
```
