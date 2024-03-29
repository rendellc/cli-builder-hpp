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

class Argument;
using Arguments = std::array<Argument, CLI_CMD_TOKENS_MAX>;
using Callback = std::function<void(Arguments)>;
class Command;
class Token;

using SizeT = CLI_SIZE_T_TYPE;

namespace constants {
constexpr const char *word = "?s";
constexpr const char *integer = "?i";
constexpr const char *decimal = "?f";

// TODO: merge these togheter
enum class SchemaType { invalid, text, argWord, argInteger, argDecimal };

} // namespace constants
//

namespace parsers {
constants::SchemaType parseType(const Token &token);
}

/* @class Token
 * Represent a token (aka a substring of either the command definition
 * or the input string.
 */
class Token {
  const char *m_raw = nullptr;
  SizeT m_len = 0;
  constants::SchemaType m_type = constants::SchemaType::invalid;

public:
  Token() = default;
  Token(const char *str, SizeT len)
      : m_raw(str), m_len(len), m_type(parsers::parseType(*this)) {}

  constants::SchemaType getSchemaType() const { return m_type; }

  const char *str() const { return m_raw; }
  SizeT len() const { return m_len; }

  bool isValid() const {
    return m_raw != nullptr && m_len > 0 &&
           m_type != constants::SchemaType::invalid;
  }

  bool operator==(const Token &other) const {
    if (!isValid() || !other.isValid()) {
      return false;
    }

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

/* @class Argument is a tagged union wrapping multiple
 * types of parsed data.
 */
class Argument {
  enum class Tag {
    none,
    string,
    integer,
    decimal,
  };
  Tag m_tag = Tag::none;

  union Data {
    char text[CLI_ARG_MAX_TEXT_LEN] = {0};
    int integer;
    float decimal;
  };
  Data m_value = {};

public:
  Argument() = default;
  bool isValid() const { return m_tag != Tag::none; }

  static Argument none() { return Argument{}; }
  static Argument integer(int value) {
    Argument a;
    a.m_tag = Tag::integer;
    a.m_value.integer = value;
    return a;
  }
  static Argument decimal(float value) {
    Argument a;
    a.m_tag = Tag::decimal;
    a.m_value.decimal = value;
    return a;
  }
  static Argument text(const Token &token) {
    Argument a;
    a.m_tag = Tag::string;
    strncpy(a.m_value.text, token.str(), token.len());
    return a;
  }

  const char *getString() const {
    CLI_ASSERT(m_tag == Tag::string,
               "trying to get non-word argument as word\n");

    return m_value.text;
  }

  int getInt() const {
    CLI_ASSERT(m_tag == Tag::integer,
               "trying to get non-int argument as integer\n");

    return m_value.integer;
  }
  float getFloat() const {
    CLI_ASSERT(m_tag == Tag::decimal,
               "trying to get non-float argument as float\n");

    return m_value.decimal;
  }
};

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
  const bool validFirst =
      first == '+' || first == '-' || ('0' <= first && first <= '9');
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

    if ('0' <= c && c <= '9') {
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

constants::SchemaType parseType(const Token &token) {
  if (std::strncmp(constants::word, token.str(), token.len()) == 0) {
    return constants::SchemaType::argWord;
  }
  if (std::strncmp(constants::integer, token.str(), token.len()) == 0) {
    return constants::SchemaType::argInteger;
  }
  if (std::strncmp(constants::decimal, token.str(), token.len()) == 0) {
    return constants::SchemaType::argDecimal;
  }

  return constants::SchemaType::text;
}

Argument tokenParser(const Token &token, const Token &inputToken) {
  CLI_ASSERT(token.isValid(), "token schema is invalid");
  CLI_ASSERT(inputToken.isValid(), "inputToken is invalid");

  switch (token.getSchemaType()) {
  case constants::SchemaType::invalid:
    return cli::Argument::none();
  case constants::SchemaType::text:
    if (token == inputToken) {
      return Argument::text(inputToken);
    }
    break;
  case constants::SchemaType::argWord:
    return Argument::text(inputToken);
    break;
  case constants::SchemaType::argInteger: {
    int value;
    if (parsers::integerParser(inputToken, value)) {
      return Argument::integer(value);
    }
    break;
  }
  case constants::SchemaType::argDecimal: {
    float value;
    if (parsers::decimalParser(inputToken, value)) {
      return Argument::decimal(value);
    }
    break;
  }
  }

  return cli::Argument::none();
}
} // namespace parsers

class Command {
  std::array<Token, CLI_CMD_TOKENS_MAX> m_tokens;
  SizeT m_numParts = 0;
  Callback m_callback = nullptr;

public:
  Command() = default;
  Command(const char *pattern, Callback callback) : m_callback(callback) {
    SizeT i = 0;
    SizeT tokenStart = 0;
    SizeT tokenLen = 0;
    while (parsers::tokenSplitter(pattern, tokenStart, tokenLen)) {
      m_tokens[i] = Token(pattern + tokenStart, tokenLen);
      tokenStart = tokenStart + tokenLen;
      i += 1;
    }
    m_numParts = i;
  }

  bool parse(const char *userInput, Arguments &args) const {
    SizeT argsFound = 0;
    SizeT inputTokenStart = 0;
    SizeT inputTokenLen = 0;

    for (const auto &commandToken : m_tokens) {
      const bool beyondFinalPart = !commandToken.isValid();
      if (beyondFinalPart) {
        break;
      }

      if (!parsers::tokenSplitter(userInput, inputTokenStart, inputTokenLen)) {
        // more tokens than present in input
        return false;
      }

      const Token inputToken =
          Token(userInput + inputTokenStart, inputTokenLen);
      inputTokenStart += inputTokenLen;
      const Argument arg = parsers::tokenParser(commandToken, inputToken);
      args[argsFound] = arg;
      argsFound++;
      if (!arg.isValid()) {
        return false;
      }
    }

    return true;
  }

  void run(const Arguments &args) const { m_callback(args); }

  bool tryRun(const char *input) const {
    std::array<Argument, CLI_CMD_TOKENS_MAX> arguments; ///< Parsed arguments
    if (!parse(input, arguments)) {
      return false;
    }

    run(arguments);
    return true;
  }
};

/*
 *
 *
 */
class CLI {
  std::array<Command, CLI_CMD_COUNT_MAX> m_commands;
  SizeT m_numberOfCommands = 0;

public:
  bool addCommand(Command cmd) {
    if (m_numberOfCommands >= m_commands.size()) {
      return false;
    }

    m_commands[m_numberOfCommands] = cmd;
    m_numberOfCommands++;
    return true;
  }

  bool addCommand(const char *pattern, Callback callback) {
    return addCommand(Command(pattern, callback));
  }

  bool run(const char *input) const {
    std::array<Argument, CLI_CMD_TOKENS_MAX> arguments; ///< Parsed arguments
    for (int i = 0; i < m_numberOfCommands; i++) {
      if (m_commands[i].parse(input, arguments)) {
        m_commands[i].run(arguments);
        return true;
      }
    }

    return false;
  }
};
} // namespace cli

#endif
