#include <iostream>

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
  const char *input = args[1].getString();
  cout << "testing integer parser on " << input << endl;
  int value = 12837912;
  const bool success =
      cli::parsers::integerParser(cli::Token(input, strlen(input)), value);
  cout << "success: " << success << endl;
  cout << "value: " << value << endl;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    return 1;
  }

  int voltage = 0;
  const auto cli =
      cli::CLI()
          .withDefaultSchemas()
          .withCommand("hello", [](cli::Arguments args) { hello(); })
          .withCommand("echo ?s",
                       [](cli::Arguments args) { echo(args[1].getString()); })
          .withCommand("pm lim vin ?i ?i",
                       [](cli::Arguments args) {
                         setLimits(args[3].getInt(), args[4].getInt());
                       })
          .withCommand(
              "ratio set ?f",
              [](cli::Arguments args) { setRatio(args[2].getFloat()); })
          .withCommand("set voltage ?i",
                       [&voltage](cli::Arguments args) {
                         cout << "set voltage called with arg 2 (int): "
                              << args[2].getInt() << endl;
                         voltage = args[2].getInt();
                       })
          .withCommand("parseint ?s", testParser);

  const char *const input = argv[1];
  if (!cli.run(input)) {
    std::cerr << "no commands matched the input: " << input << std::endl;
  }

  return 0;
}
