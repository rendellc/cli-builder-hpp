# Single file CLI builder

Goals:
- Simple API to define commands
- Easy integration into any existing project
- No dynamic allocations to enable running on microcontrollers
- Prefer simplicity over features (If more features are required, then library is small and simple enough to be modified by users)
- Safe. Invalid and malicious inputs are handled safely, and should not be visible outside the CLI library


Todo
- clean up and document
    - reduce size of public interface for classes
    - doxygen comments
- add more examples
- tests
- helptext generator
- add license
- no throwing. Always return errors, except with asserts

## Usage

Copy cli.hpp into your project and include it. See examples and tests for examples of
how to use it.
```
wget https://raw.githubusercontent.com/rendellc/single_file_cli_hpp/main/include/cli/cli.hpp
```


```


```

## Building examples and running tests

```
mkdir -p build
cd build
cmake ..
make
./tests
```
