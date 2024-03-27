#include <catch2/catch_test_macros.hpp>

#include "cli/cli.hpp"

TEST_CASE("basic usage", "[cli]") {
  using cli::Arguments;
  using cli::Cmd;

  bool wasSetByCallback = false;
  const auto cmd = Cmd({"hello"}, [&wasSetByCallback](Arguments args) {
    wasSetByCallback = true;
  });

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

  SECTION("prefix match but input is too long") {
    REQUIRE(!cmd.tryRun("helloo"));
    REQUIRE(!wasSetByCallback);
  }
}

// TEST_CASE("Tests for bugs that", "[cli usage]") {
//   using cli::Arguments;
//   using cli::Cmd;
//
//   bool wasSetByCallback = false;
//   const auto cmd = Cmd({"hello"}, [&wasSetByCallback](Arguments args) {
//     wasSetByCallback = true;
//   });
//
//   REQUIRE(cmd.tryRun("hello"));
//   REQUIRE(wasSetByCallback);
// };
