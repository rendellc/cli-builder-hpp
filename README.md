# Single file CLI builder

Goals:
- Simple API to define commands
- Easy integration into any existing project
- No dynamic allocations to enable running on microcontrollers
- Prefer simplicity over features (If more features are required, then library is easy enough to be modified by users)



Todo:
- add more examples
- tests
- helptext generator



## Usage

Copy cli.hpp into your project and include it. See examples and tests for examples of
how to use it.


## Building examples and running tests

```
mkdir -p build
cd build
cmake ..
make
./tests
```
