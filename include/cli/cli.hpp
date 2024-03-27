#pragma once

#include <array>
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

#ifndef CLI_MAX_CMD_PARTS
#define CLI_MAX_CMD_PARTS 4
#endif

#ifndef CLI_ARG_MAX_TEXT_LEN
#define CLI_ARG_MAX_TEXT_LEN 16
#endif

#ifndef CLI_SIZE_T_TYPE
#define CLI_SIZE_T_TYPE uint8_t
// #define CLI_SIZE_T_TYPE std::size_t
#endif

namespace cli {

namespace {
using SizeT = CLI_SIZE_T_TYPE;

};

class Argument {
  union Result {
    char text[CLI_ARG_MAX_TEXT_LEN] = {0};
    int integer;
    float decimal;
  };

public:
  enum class Type {
    none,
    word,    ///< ?s
    integer, ///< ?i
    decimal, ///< %d
  };
  static Argument fail() { return Argument{}; }
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
  const char *getText() const { return m_value.text; }
  int getInt() const {
    if (getType() != Type::integer) {
      CLI_WARN("trying to get non-int parameter as integer\n");
      return 0;
    }

    return m_value.integer;
  }
  float getFloat() const { return m_value.decimal; }

private:
  Type m_type = Type::none;
  Result m_value = {};
};

using Arguments = std::array<Argument, CLI_MAX_CMD_PARTS>;
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
    case SchemaType::argInteger:
      return Argument::integer(std::atoi(input));
      break;
    case SchemaType::argDecimal:
      return Argument::decimal(static_cast<float>(std::atof(input)));
      break;
    }

    return Argument::fail();
  }

private:
  const char *m_str = nullptr;
  SizeT m_partLen = 0;

  static SchemaType parseType(const char *str) {
    if (std::strcmp("?s", str) == 0) {
      return SchemaType::argString;
    }
    if (std::strcmp("?i", str) == 0) {
      return SchemaType::argInteger;
    }
    if (std::strcmp("?f", str) == 0) {
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
    CLI_ASSERT(len(), "token is an empty string");
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

class Cmd {
  std::array<Token, CLI_MAX_CMD_PARTS> m_parts;
  const SizeT m_numParts = 0;
  Callback m_callback;

public:
  Cmd(std::initializer_list<Token> parts, Callback callback)
      : m_numParts(parts.size()), m_callback(callback) {
    SizeT i = 0;
    for (const auto &part : parts) {
      m_parts[i] = part;

      i++;
      if (i >= m_numParts) {
        CLI_WARN("Cmd has more parts than array can hold. Increase "
                 "CLI_MAX_CMD_PARTS\n");
        break;
      }
    }
  }

  ///< Try to run command, return true if successful
  bool tryRun(const char *input) const {
    std::array<Argument, CLI_MAX_CMD_PARTS> arguments; ///< Parsed arguments
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
        arguments[args_found] = result;
        args_found++;
        break;
      case Argument::Type::decimal:
        CLI_DEBUG("arg %lu: parsed '%s' as decimal with '%s'\n", args_found,
                  input, part.c_str());
        arguments[args_found] = result;
        args_found++;
        break;
      case Argument::Type::word:
        arguments[args_found] = result;
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

    m_callback(arguments);
    return true;
  }
};

} // namespace cli
