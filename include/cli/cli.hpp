#pragma once

#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>

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

#ifndef CLI_CMD_TOKENS_MAX
#define CLI_CMD_TOKENS_MAX 4
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

using SizeT = CLI_SIZE_T_TYPE;

namespace constants {
constexpr const char *word = "?s";
constexpr const char *integer = "?i";
constexpr const char *decimal = "?d";

// TODO: merge these togheter
enum class SchemaType { invalid, text, argWord, argInteger, argDecimal };

} // namespace constants
//

namespace parsers {
constants::SchemaType parseType(const char *str, const SizeT len);
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
      : m_raw(str), m_len(len), m_type(parsers::parseType(str, len)) {}

  constants::SchemaType getSchemaType() const { return m_type; }

  const char *str() const { return m_raw; }
  SizeT len() const { return m_len; }

  bool isValid() const {
    return m_raw != nullptr && m_len > 0 &&
           m_type != constants::SchemaType::invalid;
  }
};

class Argument {
  /// Describe the type of a parsed argument in a tagged union
  enum class Type {
    none,
    word, ///< ?s: a single word
    // trail,   ///< ?s*: the rest of the input as a string
    integer, ///< ?i: integer 123, +4, -1, 0
    decimal, ///< ?f: decimal (cast to float): 0.1, 10.42, -123.456
  };
  Type m_type = Type::none;

  union Result {
    char text[CLI_ARG_MAX_TEXT_LEN] = {0};
    int integer;
    float decimal;
  };
  Result m_value = {};

public:
  bool isValid() const { return m_type != Type::none; }

  static Argument none() { return Argument{}; }
  static Argument integer(int value) {
    Argument a;
    a.m_type = Type::integer;
    a.m_value.integer = value;
    return a;
  }
  static Argument decimal(float value) {
    Argument a;
    a.m_type = Type::decimal;
    a.m_value.decimal = value;
    return a;
  }
  static Argument text(const char *value, SizeT len) {
    const SizeT size = static_cast<SizeT>(strcspn(value, " "));
    Argument a;
    a.m_type = Type::word;
    strncpy(a.m_value.text, value, size);
    return a;
  }

  const char *getWord() const {
    CLI_ASSERT(m_type == Type::word,
               "trying to get non-word argument as word\n");

    return m_value.text;
  }
  int getInt() const {
    CLI_ASSERT(m_type == Type::integer,
               "trying to get non-int argument as integer\n");

    return m_value.integer;
  }
  float getFloat() const {
    CLI_ASSERT(m_type == Type::decimal,
               "trying to get non-float argument as float\n");

    return m_value.decimal;
  }
};

namespace str {
bool cmp(const char *str1, const SizeT len1, const char *str2,
         const SizeT len2) {
  CLI_ASSERT(str1 != nullptr, "str1 is nullptr");
  CLI_ASSERT(str2 != nullptr, "str2 is nullptr");

  if (len1 != len2) {
    return false;
  }

  const auto len = len1;

  for (SizeT i = 0; i < len; i++) {
    if (str1[i] != str2[i]) {
      return false;
    }
  }

  return true;
}
} // namespace str

namespace parsers {
bool integerParser(const char *input, int &value) {
  // TODO: should work on tokens. some token parsing logic is
  // contained here
  CLI_ASSERT(input != nullptr, "integerParser got nullptr input");
  if (input == nullptr) {
    return false;
  }

  const char &first = *input;
  const bool validFirst =
      first == '+' || first == '-' || ('0' <= first && first <= '9');
  if (!validFirst) {
    return false;
  }

  const bool isNegative = first == '-';
  const bool skipFirst = first == '-' || first == '+';
  if (skipFirst) {
    input++;
  }

  value = 0;
  while (*input != 0 && !std::isspace(*input) && '0' <= *input &&
         *input <= '9') {
    const int digit = static_cast<int>(*input - '0');
    value = digit + 10 * value;

    input++;
  }

  const bool hadValidStopCharacter = std::isspace(*input) || *input == 0;
  if (!hadValidStopCharacter) {
    return false;
  }

  if (isNegative) {
    value = -value;
  }

  return true;
}

// TODO: work directly on Tokens, so that we do
// not have to check for spaces or 0 in input string
bool decimalParser(const char *input, float &value) {
  CLI_ASSERT(input != nullptr, "decimalParser got nullptr input");
  if (input == nullptr) {
    return false;
  }

  const char &first = *input;
  const bool validFirst =
      first == '+' || first == '-' || ('0' <= first && first <= '9');
  if (!validFirst) {
    return false;
  }

  const bool isNegative = first == '-';
  const bool skipFirst = first == '-' || first == '+';
  if (skipFirst) {
    input++;
  }

  int numberPart = 0;
  while (*input != 0 && !std::isspace(*input) && '0' <= *input &&
         *input <= '9') {
    const int digit = static_cast<int>(*input - '0');
    numberPart = digit + 10 * numberPart;

    input++;
  }
  if (!(*input == '.' || *input == 0 || std::isspace(*input))) {
    return false;
  }

  if (*input == '.') {
    input++;
  }

  int decimalPart = 0;
  SizeT decimalDivider = 1;
  while (*input != 0 && !std::isspace(*input) && '0' <= *input &&
         *input <= '9') {
    const int digit = static_cast<int>(*input - '0');
    decimalPart = digit + 10 * decimalPart;
    decimalDivider *= 10;

    input++;
  }
  if (*input != 0 && !std::isspace(*input)) {
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

constants::SchemaType parseType(const char *str, const SizeT len) {
  if (std::strncmp(constants::word, str, len) == 0) {
    return constants::SchemaType::argWord;
  }
  if (std::strncmp(constants::integer, str, len) == 0) {
    return constants::SchemaType::argInteger;
  }
  if (std::strncmp(constants::decimal, str, len) == 0) {
    return constants::SchemaType::argDecimal;
  }

  return constants::SchemaType::text;
}

Argument tokenParser(const Token &token, const Token &inputToken) {
  CLI_ASSERT(token.isValid(), "token schema is invalid");
  CLI_ASSERT(inputToken.str() != nullptr, "input is nullptr");
  CLI_ASSERT(inputToken.len(), "inputTokenLen is 0");

  switch (token.getSchemaType()) {
  case constants::SchemaType::invalid:
    return cli::Argument::none();
  case constants::SchemaType::text:
    if (str::cmp(token.str(), token.len(), inputToken.str(),
                 inputToken.len())) {
      return Argument::text(inputToken.str(), inputToken.len());
    }
    break;
  case constants::SchemaType::argWord:
    return Argument::text(inputToken.str(), inputToken.len());
    break;
  case constants::SchemaType::argInteger: {
    int value;
    if (parsers::integerParser(inputToken.str(), value)) {
      return Argument::integer(value);
    }
    break;
  }
  case constants::SchemaType::argDecimal: {
    float value;
    if (parsers::decimalParser(inputToken.str(), value)) {
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
  Callback m_callback;

public:
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

  ///< Try to run command, return true if successful

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

} // namespace cli
