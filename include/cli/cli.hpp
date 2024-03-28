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

using SizeT = CLI_SIZE_T_TYPE;

namespace constants {
constexpr const char *word = "?s";
constexpr const char *integer = "?i";
constexpr const char *decimal = "?d";
} // namespace constants

namespace parsers {
bool integerParser(const char *input, int &value) {
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

bool decimalParser(const char *input, float &value) {
  CLI_ASSERT(input != nullptr, "decimalParser got nullptr input");
  value = static_cast<float>(std::atof(input));
  return true;
}

// bool wordParser(const char *input, char *value, SizeT bufSize) {
//
//     const SizeT size = static_cast<SizeT>(strcspn(value, " "));
//     Argument a;
//     a.m_type = Type::word;
//     strncpy(a.m_value.text, value, size);
//   //
//   //
//   return false;
// }
} // namespace parsers

class Argument {
  union Result {
    char text[CLI_ARG_MAX_TEXT_LEN] = {0};
    int integer;
    float decimal;
  };

public:
  enum class Type {
    none,
    word, ///< ?s: a single word
    // trail,   ///< ?s*: the rest of the input as a string
    integer, ///< ?i: integer 123, +4, -1, 0
    decimal, ///< ?f: decimal (cast to float): 0.1, 10.42, -123.456
  };
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
  static Argument text(const char *value) {
    const SizeT size = static_cast<SizeT>(strcspn(value, " "));
    Argument a;
    a.m_type = Type::word;
    strncpy(a.m_value.text, value, size);
    return a;
  }

  Type getType() const { return m_type; }
  const char *getWord() const {
    CLI_ASSERT(getType() == Type::word,
               "trying to get non-word argument as word\n");

    return m_value.text;
  }
  int getInt() const {
    CLI_ASSERT(getType() == Type::integer,
               "trying to get non-int argument as integer\n");

    return m_value.integer;
  }
  float getFloat() const {
    CLI_ASSERT(getType() == Type::decimal,
               "trying to get non-float argument as float\n");

    return m_value.decimal;
  }

private:
  Type m_type = Type::none;
  Result m_value = {};
};

using Arguments = std::array<Argument, CLI_CMD_TOKENS_MAX>;
using Callback = std::function<void(Arguments)>;

class Token {

public:
  Token() = default;
  Token(const char *str)
      : m_str(str), m_partLen(parseLen(str)), m_type(parseType(str)) {}
  Token &operator=(const Token &rhs) {
    if (this != &rhs) {
      m_str = rhs.m_str;
      m_partLen = rhs.m_partLen;
      m_type = rhs.m_type;
    }

    return *this;
  }

  enum class SchemaType { subcommand, argString, argInteger, argDecimal };
  SchemaType getSchemaType() const { return m_type; }

  const char *c_str() const { return m_str; }
  bool isEmpty() const { return m_str == nullptr; }
  SizeT len() const { return m_partLen; }
  Argument parse(const char *input) const {
    CLI_ASSERT(!isEmpty(), "part is empty");
    CLI_ASSERT(len() > 0, "part length was zero");
    CLI_ASSERT(input != nullptr, "received nullptr input");

    switch (m_type) {
    case SchemaType::subcommand:
      if (cmp(input)) {
        return Argument::text(input);
      }
      break;
    case SchemaType::argString:
      return Argument::text(input);
      break;
    case SchemaType::argInteger: {
      int value;
      if (parsers::integerParser(input, value)) {
        return Argument::integer(value);
      }
      break;
    }
    case SchemaType::argDecimal: {
      float value;
      if (parsers::decimalParser(input, value)) {
        return Argument::decimal(value);
      }
      break;
    }
    }

    return Argument::none();
  }

private:
  const char *m_str = nullptr;
  SizeT m_partLen = 0;

  static SchemaType parseType(const char *str) {
    if (std::strcmp(constants::word, str) == 0) {
      return SchemaType::argString;
    }
    if (std::strcmp(constants::integer, str) == 0) {
      return SchemaType::argInteger;
    }
    if (std::strcmp(constants::decimal, str) == 0) {
      return SchemaType::argDecimal;
    }

    return SchemaType::subcommand;
  }

  static SizeT parseLen(const char *str) {
    CLI_ASSERT(str != nullptr, "cannot parse got nullptr string");

    return std::strlen(str);
  }

  SchemaType m_type = SchemaType::subcommand;

  bool cmp(const char *str) const {
    CLI_ASSERT(m_str != nullptr, "token is initialized with nullptr");
    CLI_ASSERT(len() > 0, "token is an empty string");
    CLI_ASSERT(str != nullptr, "cmp function got nullptr input");
    CLI_DEBUG("comparing input '%s' with token '%s'\n", str, m_str);

    const SizeT inputLen = std::strlen(str);
    if (inputLen < len()) {
      return false;
    }

    for (SizeT i = 0; i < len(); i++) {
      CLI_DEBUG("%c =? %c\n", m_str[i], str[i]);
      if (m_str[i] != str[i]) {
        return false;
      }
    }

    // NOTE: all characters in token matched
    // but input might be longer (e.g. token=help, input=helpme)
    // which should not give a match

    if (inputLen > len() && !std::isspace(str[len()])) {
      return false;
    }

    return true;
  }
};

class Command {
  std::array<Token, CLI_CMD_TOKENS_MAX> m_parts;
  const SizeT m_numParts = 0;
  Callback m_callback;

public:
  Command(std::initializer_list<Token> parts, Callback callback)
      : m_numParts(parts.size()), m_callback(callback) {
    SizeT i = 0;
    for (const auto &part : parts) {
      m_parts[i] = part;

      i++;
      if (i >= m_numParts) {
        CLI_WARN("Cmd has more parts than array can hold. Increase "
                 "CLI_CMD_TOKENS_MAX\n");
        break;
      }
    }
  }

  ///< Try to run command, return true if successful

  bool parse(const char *input, Arguments &args) const {
    SizeT args_found = 0;

    for (const auto &part : m_parts) {
      const bool beyondFinalPart = part.isEmpty();
      if (beyondFinalPart) {
        break;
      }

      const auto result = part.parse(input);
      switch (result.getType()) {
      case Argument::Type::none:
        CLI_DEBUG("cant parse '%s' according to part '%s'\n", input,
                  part.c_str());
        return false;
      case Argument::Type::integer:
        CLI_DEBUG("arg %lu: parsed '%s' as integer with '%s'\n", args_found,
                  input, part.c_str());
        args[args_found] = result;
        args_found++;
        break;
      case Argument::Type::decimal:
        CLI_DEBUG("arg %lu: parsed '%s' as decimal with '%s'\n", args_found,
                  input, part.c_str());
        args[args_found] = result;
        args_found++;
        break;
      case Argument::Type::word:
        args[args_found] = result;
        CLI_DEBUG("arg %lu: parsed '%s' as text with '%s'\n", args_found, input,
                  part.c_str());
        args_found++;
        break;
      }

      // move across current word
      while (*input != 0 && !std::isspace(*input)) {
        input++;
      }
      // move to next word
      while (*input != 0 && std::isspace(*input)) {
        input++;
      }
    }
    return true;
  }

  bool tryRun(const char *input) const {
    std::array<Argument, CLI_CMD_TOKENS_MAX> arguments; ///< Parsed arguments

    if (!parse(input, arguments)) {
      return false;
    }
    m_callback(arguments);
    return true;
  }
};

} // namespace cli
