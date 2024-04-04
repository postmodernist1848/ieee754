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
