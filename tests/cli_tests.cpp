#include <catch2/catch_test_macros.hpp>

#include <cli/cli.hpp>

TEST_CASE("basic usage", "[cli]") {
  using cli::Arguments;
  using cli::Cmd;

  bool wasSetByCallback = false;
  const auto cmd = Cmd({"hello"}, [&wasSetByCallback](Arguments args) {
    wasSetByCallback = true;
  });

  int voltage = 0;
  const auto setVoltageCmd =
      Cmd({"set", "voltage", "?i"},
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
    REQUIRE(setVoltageCmd.tryRun("set voltage 42"));
    REQUIRE(voltage == 42);
  }

  SECTION("multipart command non match", "[cli]") {
    REQUIRE(!setVoltageCmd.tryRun("set voltage not_int"));
  }
}

TEST_CASE("tests for former bugs", "[cli]") {
  using cli::Arguments;
  using cli::Cmd;

  bool wasSetByCallback = false;
  const auto cmd = Cmd({"hello"}, [&wasSetByCallback](Arguments args) {
    wasSetByCallback = true;
  });
  const auto setVoltageCmd = Cmd({"set", "voltage", "?i"}, [&](Arguments args) {
    wasSetByCallback = true;
  });

  SECTION("prefix match but input is too long") {
    REQUIRE(!cmd.tryRun("helloo"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("invalid int parsing doesn't fail") {
    REQUIRE(!setVoltageCmd.tryRun("set voltage not_int"));
    REQUIRE(!wasSetByCallback);
  }
};

TEST_CASE("integer parser", "[cli]") {
  int value = 1231241241; // value not expected by any tests
                          //
  SECTION("normal usage") {
    REQUIRE(cli::integerParser("0", value));
    REQUIRE(value == 0);

    REQUIRE(cli::integerParser("1", value));
    REQUIRE(value == 1);
    REQUIRE(cli::integerParser("+63", value));
    REQUIRE(value == 63);

    REQUIRE(cli::integerParser("-2", value));
    REQUIRE(value == -2);

    REQUIRE(cli::integerParser("-0", value));
    REQUIRE(value == 0);

    REQUIRE(cli::integerParser("-1043", value));
    REQUIRE(value == -1043);
  }

  SECTION("invalid inputs should fail") {
    REQUIRE(!cli::integerParser(nullptr, value));
    REQUIRE(!cli::integerParser("", value));
    REQUIRE(!cli::integerParser(" ", value));
    REQUIRE(!cli::integerParser("1+2", value));
    REQUIRE(!cli::integerParser("text", value));
    REQUIRE(!cli::integerParser("10.0", value));
  }
};
