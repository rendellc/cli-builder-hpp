/* @file Hello World program for CLI
 *  Run with ./hello_world "hello"
 */

#include <iostream>

#include <cli/cli.hpp>

void hello() { std::cout << "welcome to the CLI" << std::endl; }

int main(int argc, char *argv[]) {
  if (argc != 2) {
    return 1;
  }
  const char *const input = argv[1];

  // Create a CLI which can hold up to 4 commands
  cli::CLI<4> cli;
  cli.addCommand("hello", [](cli::Arguments args) { hello(); });

  // Run CLI with input
  if (!cli.run(input)) {
    std::cout << "no commands matched the input: " << input << std::endl;
  }

  return 0;
}
