# Single file CLI builder

Goals:
- Simple API to define commands
- Easy integration into any existing project
- No dynamic allocations to enable running on microcontrollers
- Prefer simplicity over features (If more features are required, then library is small and simple enough to be modified by users)


Todo
- add root CLI class to hold all commands and serve as main access point
    - maxCommands
    - addCommand
    - tryRun
- clean up and document
    - Be consistent about Cmd or Command
    - initialize token from c_str instead of initializer_list
    - reduce size of public interface for classes
    - doxygen comments
- add more examples
- improve consistency between SchemaType and Type. Can they be the same?
- tests
- helptext generator
- add license



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
