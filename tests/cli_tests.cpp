#include <catch2/catch_test_macros.hpp>

#include <cli/cli.hpp>
#include <limits>

TEST_CASE("usage through CLI class", "[cli]") {
  using cli::Arguments;
  using cli::CLI;

  bool wasSetByCallback = false;
  int voltage = 0;
  auto cli = CLI()
                 .withDefaultSchemas()
                 .withCommand("hello",
                              [&wasSetByCallback](Arguments args) {
                                wasSetByCallback = true;
                              })
                 .withCommand("set voltage ?i", [&voltage](Arguments args) {
                   voltage = args[2].get<int>();
                 });

  SECTION("callback is called and variable is modified") {
    REQUIRE(cli.run("hello"));
    REQUIRE(wasSetByCallback);
  }

  SECTION("input doesn't match command") {
    REQUIRE(!cli.run("echo"));
    REQUIRE(!cli.run("help"));
    REQUIRE(!cli.run("helpo"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("empty input is handled") {
    REQUIRE(!cli.run(""));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("almost match") {
    REQUIRE(!cli.run("hell"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("multipart command", "[cli]") {
    REQUIRE(cli.run("set voltage 42"));
    REQUIRE(voltage == 42);
  }

  SECTION("multipart command non match", "[cli]") {
    REQUIRE(!cli.run("set voltage not_int"));
  }
}

TEST_CASE("tests for former bugs", "[cli]") {
  using cli::Arguments;
  using cli::CLI;

  bool wasSetByCallback = false;
  int lim1, lim2;

  const auto cli =
      CLI()
          .withDefaultSchemas()
          .withCommand(
              "hello",
              [&wasSetByCallback](Arguments args) { wasSetByCallback = true; })
          .withCommand("set voltage ?i",
                       [&](Arguments args) { wasSetByCallback = true; })
          .withCommand("pm lim vin ?i ?i", [&](cli::Arguments args) {
            lim1 = args[3].get<int>();
            lim2 = args[4].get<int>();
          });

  SECTION("prefix match but input is too long") {
    REQUIRE(!cli.run("helloo"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("invalid int parsing doesn't fail") {
    REQUIRE(!cli.run("set voltage not_int"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("incomplete input shold fail") {
    REQUIRE(!cli.run("set voltage"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("pm vin lim 3 5 used to fail") {
    REQUIRE(cli.run("pm lim vin 3 5"));
    REQUIRE(lim1 == 3);
    REQUIRE(lim2 == 5);
  }

  // NOTE: what happens when input matches command but is longer?
  // Ie command is prefix of input
  SECTION("input is too long") {
    REQUIRE(!cli.run("pm lim vin 3 5 extra stuff"));
    REQUIRE(!cli.run("hello hello"));
    REQUIRE(!wasSetByCallback);
  }

  SECTION("invalid inputs should fail") {
    REQUIRE(!cli.run("aslkadsfklas"));
    REQUIRE(!cli.run(nullptr));
  }
};

TEST_CASE("integer parser") {
  int value = 1231241241; // value not expected by any tests
  const auto cli = cli::CLI().withDefaultSchemas().withCommand(
      "test ?i", [&](cli::Arguments args) { value = args[1].get<int>(); });

  SECTION("normal usage") {
    REQUIRE(cli.run("test 0"));
    REQUIRE(value == 0);

    REQUIRE(cli.run("test 1"));
    REQUIRE(value == 1);
    REQUIRE(cli.run("test +63"));
    REQUIRE(value == 63);

    REQUIRE(cli.run("test -2"));
    REQUIRE(value == -2);

    REQUIRE(cli.run("test -0"));
    REQUIRE(value == 0);

    REQUIRE(cli.run("test -1043"));
    REQUIRE(value == -1043);
  }

  SECTION("invalid inputs should fail") {
    REQUIRE(!cli.run("test "));
    REQUIRE(!cli.run("test      not_int 123"));
    REQUIRE(!cli.run("test 1+2"));
    REQUIRE(!cli.run("test text"));
    REQUIRE(!cli.run("test 10.0"));
  }
};

TEST_CASE("decimal parser", "[cli]") {
  float value = 123.4560123; // value not expected by any tests
  const auto cli = cli::CLI().withDefaultSchemas().withCommand(
      "test ?f", [&](cli::Arguments args) { value = args[1].get<float>(); });
  const auto isEqual = [](float a, float b) {
    // return std::abs(a - b) <= 0.00001f;
    return std::abs(a - b) <= std::numeric_limits<float>::epsilon();
  };

  SECTION("normal usage") {
    REQUIRE(cli.run("test 0"));
    REQUIRE(isEqual(0.0, value));

    REQUIRE(cli.run("test 0.0"));
    REQUIRE(isEqual(0.0, value));

    REQUIRE(cli.run("test 1"));
    REQUIRE(isEqual(1.0, value));
    REQUIRE(cli.run("test +63"));
    REQUIRE(isEqual(63.0, value));

    REQUIRE(cli.run("test -2"));
    REQUIRE(isEqual(-2.0, value));

    REQUIRE(cli.run("test -0"));
    REQUIRE(isEqual(-0.0, value));

    REQUIRE(cli.run("test -1043"));
    REQUIRE(isEqual(-1043.0, value));

    REQUIRE(cli.run("test 10.0"));
    REQUIRE(isEqual(10.0, value));

    REQUIRE(cli.run("test 3.141516"));
    REQUIRE(isEqual(3.141516, value));
  }

  SECTION("invalid inputs should fail") {
    REQUIRE(!cli.run(nullptr));
    REQUIRE(!cli.run("test nan"));     // not supporting nan
    REQUIRE(!cli.run("test inf"));     // not supporting inf
    REQUIRE(!cli.run("test 1.0 2.0")); // too many input tokens
    REQUIRE(!cli.run("test "));        // missing number
    REQUIRE(!cli.run("test 1+2"));     // invalid
    REQUIRE(!cli.run("test text"));    // invalid
  }
};

TEST_CASE("string parser", "[cli]") {
  using cli::Arguments;
  using cli::CLI;

  const auto isEqual = [](const char *a, const char *b) {
    if (a == nullptr || b == nullptr) {
      return false;
    }
    if (a == b) {
      return true;
    }
    int i = 0;
    while (a[i] == b[i] && a[i] != 0 && b[i] != 0) {
      i++;
    }

    if (a[i] != b[i]) {
      return false;
    }

    return true;
  };

  SECTION("compare with known string", "[string]") {
    const char *testString = "string test_compare";
    bool wasEqual = true;
    const auto cli = CLI().withDefaultSchemas().withCommand(
        "string ?s", [&](Arguments args) {
          wasEqual = isEqual(args[1].getString(), "test_compare");
        });
    REQUIRE(cli.run("string hello"));
    REQUIRE(!wasEqual);
    REQUIRE(cli.run("string testtest"));
    REQUIRE(!wasEqual);
    REQUIRE(cli.run("string test_erapmoc"));
    REQUIRE(!wasEqual);
    REQUIRE(cli.run(testString));
    REQUIRE(wasEqual);
    REQUIRE(cli.run("string test_compare_"));
    REQUIRE(!wasEqual);
    REQUIRE(cli.run("string test_compare"));
    REQUIRE(wasEqual);
  }
};
