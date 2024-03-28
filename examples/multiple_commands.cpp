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
#include <cli/cli.hpp>

using std::cout;
using std::endl;

void hello() { cout << "welcome to the CLI" << endl; }

void echo(const char *msg) { cout << "echo: '" << msg << "'" << endl; }

void setLimits(int min, int max) {
  cout << "int limits set to [" << min << ", " << max << "]" << endl;
}

void setRatio(float ratio) { cout << "ratio set to " << ratio << endl; }

void testParser(cli::Arguments args) {
  const char *input = args[1].getWord();
  cout << "testing integer parser on " << input << endl;
  int value = 12837912;
  const bool success = cli::parsers::integerParser(input, value);
  cout << "success: " << success << endl;
  cout << "value: " << value << endl;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    return 1;
  }

  const cli::Command helloCmd({"hello"}, [](cli::Arguments args) { hello(); });
  const cli::Command echoCmd(
      {"echo", "?s"}, [](cli::Arguments args) { echo(args[1].getWord()); });
  const cli::Command limitCmd({"pm", "lim", "vin", "?i", "?i"},
                              [](cli::Arguments args) {
                                setLimits(args[3].getInt(), args[4].getInt());
                              });
  const cli::Command ratioCmd({"ratio", "set", "?f"}, [](cli::Arguments args) {
    setRatio(args[2].getFloat());
  });
  int voltage = 0;
  const cli::Command setVoltageCmd(
      {"set", "voltage", "?i"}, [&voltage](cli::Arguments args) {
        cout << "set voltage called with arg 2 (int): " << args[2].getInt()
             << endl;
        voltage = args[2].getInt();
      });
  const cli::Command parseIntCmd({"parseint", "?s"}, testParser);

  const char *const input = argv[1];
  if (helloCmd.tryRun(input)) {
  } else if (echoCmd.tryRun(input)) {
  } else if (limitCmd.tryRun(input)) {
  } else if (ratioCmd.tryRun(input)) {
  } else if (setVoltageCmd.tryRun(input)) {
  } else if (parseIntCmd.tryRun(input)) {
  } else {
    std::cerr << "no commands matched the input: " << input << std::endl;
  }

  return 0;
}
