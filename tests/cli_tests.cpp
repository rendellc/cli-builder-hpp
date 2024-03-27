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
  int voltage = 0;
  const auto setVoltageCmd = Cmd({"set", "voltage", "?i"}, [&](Arguments args) {
    wasSetByCallback = true;
    voltage = args[2].getInt();
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
