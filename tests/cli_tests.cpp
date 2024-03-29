#include <catch2/catch_test_macros.hpp>

#include <cli/cli.hpp>
#include <limits>

TEST_CASE("basic usage", "[cli]") {
  using cli::Arguments;
  using cli::Command;

  bool wasSetByCallback = false;
  const auto cmd = Command("hello", [&wasSetByCallback](Arguments args) {
    wasSetByCallback = true;
  });

  int voltage = 0;
  const auto setVoltageCommand =
      Command("set voltage ?i",
              [&voltage](Arguments args) { voltage = args[2].getInt(); });

  SECTION("callback is called and variable is modified") {
    REQUIRE(cmd.tryRun("hello"));
    REQUIRE(wasSetByCallback);
  }

  SECTION("input doesn't match command") {
    REQUIRE(!cmd.tryRun("echo"));
    REQUIRE(!cmd.tryRun("help"));
    REQUIRE(!cmd.tryRun("helpo"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("empty input is handled") {
    REQUIRE(!cmd.tryRun(""));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("almost match") {
    REQUIRE(!cmd.tryRun("hell"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("multipart command", "[cli]") {
    REQUIRE(setVoltageCommand.tryRun("set voltage 42"));
    REQUIRE(voltage == 42);
  }

  SECTION("multipart command non match", "[cli]") {
    REQUIRE(!setVoltageCommand.tryRun("set voltage not_int"));
  }
}

TEST_CASE("tests for former bugs", "[cli]") {
  using cli::Arguments;
  using cli::Command;

  bool wasSetByCallback = false;
  const auto cmd = Command("hello", [&wasSetByCallback](Arguments args) {
    wasSetByCallback = true;
  });
  const auto setVoltageCommand = Command(
      "set voltage ?i", [&](Arguments args) { wasSetByCallback = true; });

  int lim1, lim2;
  const cli::Command limitCmd("pm lim vin ?i ?i", [&](cli::Arguments args) {
    lim1 = args[3].getInt();
    lim2 = args[4].getInt();
  });

  SECTION("prefix match but input is too long") {
    REQUIRE(!cmd.tryRun("helloo"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("invalid int parsing doesn't fail") {
    REQUIRE(!setVoltageCommand.tryRun("set voltage not_int"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("incomplete input shold fail") {
    REQUIRE(!setVoltageCommand.tryRun("set voltage"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("pm vin lim 3 5 used to fail") {
    REQUIRE(limitCmd.tryRun("pm lim vin 3 5"));
    REQUIRE(lim1 == 3);
    REQUIRE(lim2 == 5);
  }

  // NOTE: what happens when input matches command but is longer?
  // Ie command is prefix of input
};

TEST_CASE("integer parser", "[cli]") {
  using cli::Token;
  using cli::parsers::integerParser;
  int value = 1231241241; // value not expected by any tests

  SECTION("normal usage") {
    REQUIRE(integerParser(Token("0", 1), value));
    REQUIRE(value == 0);

    REQUIRE(integerParser(Token("1", 1), value));
    REQUIRE(value == 1);
    REQUIRE(integerParser(Token("+63", 3), value));
    REQUIRE(value == 63);

    REQUIRE(integerParser(Token("-2", 2), value));
    REQUIRE(value == -2);

    REQUIRE(integerParser(Token("-0", 2), value));
    REQUIRE(value == 0);

    REQUIRE(integerParser(Token("-1043", 5), value));
    REQUIRE(value == -1043);
  }

  SECTION("invalid inputs should fail") {
    REQUIRE(!integerParser(Token(), value));
    REQUIRE(!integerParser(Token("", 0), value));
    REQUIRE(!integerParser(Token(" ", 1), value));
    REQUIRE(!integerParser(Token("1+2", 3), value));
    REQUIRE(!integerParser(Token("text", 4), value));
    REQUIRE(!integerParser(Token("10.0", 4), value));
  }
};

TEST_CASE("decimal parser", "[cli]") {
  using cli::Token;
  using cli::parsers::decimalParser;
  float value = 123.4560123;
  const auto isEqual = [](float a, float b) {
    // return std::abs(a - b) <= 0.00001f;
    return std::abs(a - b) <= std::numeric_limits<float>::epsilon();
  };

  SECTION("normal usage") {
    REQUIRE(decimalParser(Token("0", 1), value));
    REQUIRE(isEqual(0.0, value));

    REQUIRE(decimalParser(Token("0.0", 3), value));
    REQUIRE(isEqual(0.0, value));

    REQUIRE(decimalParser(Token("1", 1), value));
    REQUIRE(isEqual(1.0, value));
    REQUIRE(decimalParser(Token("+63", 3), value));
    REQUIRE(isEqual(63.0, value));

    REQUIRE(decimalParser(Token("-2", 2), value));
    REQUIRE(isEqual(-2.0, value));

    REQUIRE(decimalParser(Token("-0", 2), value));
    REQUIRE(isEqual(-0.0, value));

    REQUIRE(decimalParser(Token("-1043", 5), value));
    REQUIRE(isEqual(-1043.0, value));

    REQUIRE(decimalParser(Token("10.0", 4), value));
    REQUIRE(isEqual(10.0, value));
  }

  SECTION("invalid inputs should fail") {
    REQUIRE(!decimalParser(Token(), value));
    REQUIRE(!decimalParser(Token("", 0), value));
    REQUIRE(!decimalParser(Token(" ", 1), value));
    REQUIRE(!decimalParser(Token("1+2", 3), value));
    REQUIRE(!decimalParser(Token("text", 4), value));
  }
};
