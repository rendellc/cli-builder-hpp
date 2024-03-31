/*
Copyright © 2024 Rendell Cale

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef CLI_HPP_
#define CLI_HPP_

#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>

#define CLI_LOG_NOOP(format, ...)                                              \
  do {                                                                         \
  } while (false);

#ifndef CLI_DEBUG
#define CLI_DEBUG(format, ...) CLI_LOG_NOOP(format, ##__VA_ARGS__)
#endif

#ifndef CLI_WARN
#define CLI_WARN(format, ...) CLI_LOG_NOOP(format, ##__VA_ARGS__)
#endif

#ifndef CLI_ASSERT
#define CLI_ASSERT(must_be_true, format, ...)                                  \
  if (!(must_be_true)) {                                                       \
    CLI_WARN("assertion failed")                                               \
    CLI_WARN(format, ##__VA_ARGS__)                                            \
    throw std::exception();                                                    \
  }
#endif

#ifndef CLI_CMD_COUNT_MAX
#define CLI_CMD_COUNT_MAX 16
#endif

#ifndef CLI_SCHEMAS_COUNT_MAX
#define CLI_SCHEMAS_COUNT_MAX 4
#endif

#ifndef CLI_CMD_TOKENS_MAX
#define CLI_CMD_TOKENS_MAX 16
#endif

#ifndef CLI_ARG_MAX_TEXT_LEN
#define CLI_ARG_MAX_TEXT_LEN 16
#endif

#ifndef CLI_SIZE_T_TYPE
#define CLI_SIZE_T_TYPE uint8_t
#endif

namespace cli {

using SizeT = CLI_SIZE_T_TYPE;
template <typename T, SizeT N> class FixedVector {
  std::array<T, N> m_array;
  SizeT m_len = 0;

public:
  SizeT size() const { return m_len; }

  void clear() { m_len = 0; }

  bool push_back(T value) {
    if (m_len >= N) {
      return false;
    }
    m_array[m_len] = value;
    m_len++;
    return true;
  }

  const T &operator[](SizeT i) const { return m_array[i]; }
  T &operator[](SizeT i) { return m_array[i]; }
};

class Argument;
// using Arguments = std::array<Argument, CLI_CMD_TOKENS_MAX>;
using Arguments = FixedVector<Argument, CLI_CMD_TOKENS_MAX>;
using Callback = std::function<void(Arguments)>;
class Command;
// std::array<Command, CLI_CMD_COUNT_MAX>;
using Commands = FixedVector<Command, CLI_CMD_COUNT_MAX>;
class Token;
using Tokens = FixedVector<Token, CLI_CMD_TOKENS_MAX>;
class Schema;
using Schemas = FixedVector<Schema, CLI_SCHEMAS_COUNT_MAX>;
using TokenParser = std::function<bool(const Token &, Argument &)>;

namespace parsers {
bool integerParser(const Token &token, int &value);
bool decimalParser(const Token &token, float &value);
bool tokenSplitter(const char *input, SizeT &tokenStart, SizeT &tokenLen);
Tokens tokenParser(const char *str);
Argument argumentParser(const Schemas &schemas, const Token &token,
                        const Token &inputToken);
} // namespace parsers

using Tag = uint8_t;
namespace constants {
constexpr Tag tagInvalid = 0;
constexpr Tag tagInt = tagInvalid + 1;
constexpr Tag tagFloat = tagInt + 1;
constexpr Tag tagString = tagFloat + 1;
} // namespace constants

namespace str {
bool isInt(char c);
int toInt(char c);
} // namespace str

/* @class Token
 * Represent a token (aka a substring of either the command definition
 * or the input string.
 */
class Token {
  const char *m_raw = nullptr;
  SizeT m_len = 0;

public:
  Token() = default;
  Token(const char *str, SizeT len) : m_raw(str), m_len(len) {}

  const char *str() const { return m_raw; }
  SizeT len() const { return m_len; }

  bool isValid() const { return m_raw != nullptr && m_len > 0; }

  bool operator==(const Token &other) const {
    if (len() != other.len()) {
      return false;
    }

    for (SizeT i = 0; i < len(); i++) {
      if (str()[i] != other.str()[i]) {
        return false;
      }
    }

    return true;
  }
};

class Schema {
  Token m_pattern;
  TokenParser m_parser = nullptr;

public:
  Schema() = default;
  Schema(const char *pattern, TokenParser parser)
      : m_pattern(pattern, std::strlen(pattern)), m_parser(parser) {}

  bool isSchema(const Token &commandToken) const {
    return m_pattern == commandToken;
  }

  bool parse(const Token &inputToken, Argument &arg) const {
    if (m_parser == nullptr) {
      return false;
    }
    return m_parser(inputToken, arg);
  }
};

/* @class Argument is a tagged union wrapping multiple
 * types of parsed data.
 */
class Argument {
public:
  Tag m_tag = constants::tagInvalid;

  union Data {
    char text[CLI_ARG_MAX_TEXT_LEN] = {0};
    int integer;
    float decimal;
    uint64_t raw64;
  };
  Data m_value = {};

public:
  Argument() = default;

  bool isValid() const { return m_tag != constants::tagInvalid; }

  template <typename T> static Argument create(Tag tag, T value) {
    T *valuePtr = &value;
    uint64_t *rawPtr = (uint64_t *)(valuePtr);

    Argument a;
    a.m_tag = tag;
    a.m_value.raw64 = *rawPtr;
    return a;
  }

  template <typename T> T get() const {
    T *ptr = (T *)(&m_value.raw64);
    return *ptr;
  }

  template <typename T> T get(Tag tag) const {
    if (m_tag != tag) {
      CLI_WARN("get on non matching tag");
      return T();
    }
    const uint64_t *rawPtr = &m_value.raw64;
    T *ptr = (T *)(rawPtr);
    return *ptr;
  }

  // static Argument none() { return Argument{}; }

  // static Argument integer(int value) {
  //   return Argument::with(constants::tagInt, value);
  // }

  // static Argument decimal(float value) {
  //   return Argument::with(constants::tagFloat, value);
  // }

  static Argument text(const Token &token) {
    Argument a;
    a.m_tag = constants::tagString;
    strncpy(a.m_value.text, token.str(), token.len());
    return a;
  }

  const char *getString() const {
    if (m_tag != constants::tagString) {
      CLI_WARN("trying to get non-word argument as word\n");
      return "";
    }

    return m_value.text;
  }

  // int getInt() const {
  //   if (m_tag != Tag::integer) {
  //     CLI_WARN("trying to get non-int argument as integer\n");
  //     return 0;
  //   }

  //   return m_value.integer;
  // }
  // float getFloat() const {
  //   if (m_tag != Tag::decimal) {
  //     CLI_WARN("trying to get non-float argument as float\n");
  //     return 0.0;
  //   }

  //   return m_value.decimal;
  // }
};

static const Schema textSchema("?s", [](const Token &input, Argument &result) {
  result = Argument::text(input);
  return true;
});

static const Schema integerSchema("?i", [](const Token &input,
                                           Argument &result) {
  int value;
  if (!parsers::integerParser(input, value)) {
    return false;
  }
  result = Argument::create(constants::tagInt, value);
  return true;
});

static const Schema decimalSchema("?f", [](const Token &input,
                                           Argument &result) {
  float value;
  if (!parsers::decimalParser(input, value)) {
    return false;
  }
  result = Argument::create(constants::tagFloat, value);
  return true;
});

namespace str {
bool isInt(char c) { return '0' <= c && c <= '9'; }
int toInt(char c) { return static_cast<int>(c - '0'); }
} // namespace str

namespace parsers {
bool integerParser(const Token &token, int &value) {
  if (!token.isValid()) {
    return false;
  }
  const char &first = *token.str();
  const bool validFirst = first == '+' || first == '-' || str::isInt(first);
  if (!validFirst) {
    return false;
  }

  const bool isNegative = first == '-';
  const bool skipFirst = first == '-' || first == '+';
  SizeT i = 0;
  if (skipFirst) {
    i = 1;
  }
  value = 0;
  for (; i < token.len(); i++) {
    const char c = *(token.str() + i);
    if (!str::isInt(c)) {
      return false;
    }
    const int digit = str::toInt(c);
    value = digit + 10 * value;
  }

  if (isNegative) {
    value = -value;
  }

  return true;
}

bool decimalParser(const Token &token, float &value) {
  if (!token.isValid()) {
    return false;
  }
  const char &first = *token.str();
  const bool validFirst = first == '+' || first == '-' || str::isInt(first);
  if (!validFirst) {
    return false;
  }

  const bool isNegative = first == '-';
  const bool skipFirst = first == '-' || first == '+';
  SizeT i = 0;
  if (skipFirst) {
    i = 1;
  }

  enum class ParserState { number, decimal, failed };
  ParserState state = ParserState::number;

  int numberPart = 0;
  int decimalPart = 0;
  int decimalDivider = 1;
  for (; i < token.len(); i++) {
    const char c = token.str()[i];
    if (c == '.') {
      if (state == ParserState::number) {
        state = ParserState::decimal;
      } else {
        state = ParserState::failed;
      }
      continue;
    }

    if (str::isInt(c)) {
      const int digit = str::toInt(c);
      if (state == ParserState::number) {
        numberPart = digit + 10 * numberPart;
      }
      if (state == ParserState::decimal) {
        decimalPart = digit + 10 * decimalPart;
        decimalDivider *= 10;
      }

      continue;
    }

    state = ParserState::failed;
    break;
  }

  if (state == ParserState::failed) {
    return false;
  }

  value = numberPart + static_cast<float>(decimalPart) / decimalDivider;
  if (isNegative) {
    value = -value;
  }
  return true;
}

bool tokenSplitter(const char *input, SizeT &tokenStart, SizeT &tokenLen) {
  // begin looking at tokenStart
  while (std::isspace(*(input + tokenStart))) {
    tokenStart++;
  }
  if (*(input + tokenStart) == 0) {
    return false;
  }

  tokenLen = 0;
  while (!std::isspace(*(input + tokenStart + tokenLen)) &&
         *(input + tokenStart + tokenLen) != 0) {
    tokenLen++;
  }

  if (tokenLen == 0) {
    return false;
  }

  return true;
}

Argument argumentParser(const Schemas &schemas, const Token &commandToken,
                        const Token &inputToken) {
  CLI_ASSERT(commandToken.isValid(), "commandToken is invalid");
  CLI_ASSERT(inputToken.isValid(), "inputToken is invalid");

  for (SizeT i = 0; i < schemas.size(); i++) {
    if (!schemas[i].isSchema(commandToken)) {
      continue;
    }

    Argument arg;
    if (!schemas[i].parse(inputToken, arg)) {
      continue;
    }
    return arg;
  }

  if (inputToken == commandToken) {
    return Argument::text(inputToken);
  }

  return Argument();
}

Tokens tokenParser(const char *str) {
  Tokens tokens;
  SizeT tokenStart = 0;
  SizeT tokenLen = 0;
  while (parsers::tokenSplitter(str, tokenStart, tokenLen)) {
    tokens.push_back(Token(str + tokenStart, tokenLen));
    tokenStart = tokenStart + tokenLen;
  }

  return tokens;
}
} // namespace parsers

class Command {
  Callback m_callback = nullptr;
  Tokens m_patternTokens;

public:
  Command() = default;
  Command(const char *pattern, Callback callback)
      : m_callback(callback), m_patternTokens(parsers::tokenParser(pattern)) {}

  bool parse(const Schemas &schemas, const Tokens &inputTokens,
             Arguments &args) const {
    args.clear();

    if (inputTokens.size() != m_patternTokens.size()) {
      return false;
    }

    for (SizeT i = 0; i < m_patternTokens.size(); i++) {
      const auto &commandToken = m_patternTokens[i];
      const auto &inputToken = inputTokens[i];
      const Argument arg =
          parsers::argumentParser(schemas, commandToken, inputToken);
      if (!arg.isValid()) {
        return false;
      }
      args.push_back(arg);
    }

    return true;
  }

  void run(const Arguments &args) const { m_callback(args); }

  void getHelp(std::function<void(const char *, int)> writer) const {
    for (SizeT i = 0; i < m_patternTokens.size(); i++) {
      const auto &t = m_patternTokens[i];
      writer(t.str(), t.len());
      writer(" ", 1);
    }
  }
};

/*
 *
 */
class CLI {
  Commands m_commands;
  Schemas m_schemas;

public:
  CLI withDefaultSchemas() {
    return std::move(withSchema(integerSchema)
                         .withSchema(decimalSchema)
                         .withSchema(textSchema));
  }

  CLI withSchema(Schema schema) {
    m_schemas.push_back(schema);
    return std::move(*this);
  }
  CLI withSchema(const char *pattern, TokenParser parser) {
    return withSchema(Schema(pattern, parser));
  }

  CLI withCommand(const char *pattern, Callback callback) {
    m_commands.push_back(Command(pattern, callback));
    return std::move(*this);
  }

  bool run(const char *input) const {
    if (input == nullptr) {
      return false;
    }

    Tokens inputTokens = parsers::tokenParser(input);
    for (int i = 0; i < m_commands.size(); i++) {
      Arguments arguments;
      if (m_commands[i].parse(m_schemas, inputTokens, arguments)) {
        m_commands[i].run(arguments);
        return true;
      }
    }

    return false;
  }

  void getHelp(std::function<void(const char *, int)> writer) const {
    for (int i = 0; i < m_commands.size(); i++) {
      m_commands[i].getHelp(writer);
      writer("\n", 1);
    }
  }
};
} // namespace cli

#endif
