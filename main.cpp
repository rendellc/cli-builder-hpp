#include <iostream>

/*
#define CLI_DEBUG(format, ...)                                                 \
  printf("debug: ");                                                           \
  printf(format, ##__VA_ARGS__)
#define CLI_WARN(format, ...)                                                  \
  printf("warn: ");                                                            \
  printf(format, ##__VA_ARGS__)
*/
#define CLI_MAX_CMD_PARTS 8
#include "cli.hpp"

using std::cout;
using std::endl;

void hello() { cout << "welcome to the CLI" << endl; }

void echo(const char *msg) { cout << "echo: '" << msg << "'" << endl; }

void setLimits(int min, int max) {
  cout << "int limits set to [" << min << ", " << max << "]" << endl;
}

void setRatio(float ratio) { cout << "ratio set to " << ratio << endl; }

int main(int argc, char *argv[]) {
  if (argc != 2) {
    return 1;
  }

  const cli::Cmd helloCmd({"hello"}, [](cli::Arguments args) { hello(); });
  const cli::Cmd echoCmd({"echo", "?s"},
                         [](cli::Arguments args) { echo(args[1].getText()); });
  const cli::Cmd limitCmd({"pm", "lim", "vin", "?i", "?i"},
                          [](cli::Arguments args) {
                            setLimits(args[3].getInt(), args[4].getInt());
                          });
  const cli::Cmd ratioCmd({"ratio", "set", "?f"}, [](cli::Arguments args) {
    setRatio(args[2].getFloat());
  });

  const char *const input = argv[1];
  if (helloCmd.tryRun(input)) {
  } else if (echoCmd.tryRun(input)) {
  } else if (limitCmd.tryRun(input)) {
  } else if (ratioCmd.tryRun(input)) {
  } else {
    std::cerr << "no commands matched the input: " << input << std::endl;
  }

  return 0;
}
