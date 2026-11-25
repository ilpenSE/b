#pragma once
#include "common.hpp"
#include <format>
#include <cstddef>

enum class TokenType {
  // Single character tokens
  Colon = ':', Eq = '=', LessThan = '<', GreaterThan = '>',
  OCurly = '{', CCurly = '}', OBracket = '[', CBracket = ']',
  OParen = '(', CParen = ')', Semicolon = ';', Not = '!',
  Asterisk = '*', Slash = '/', Comma = ',', Ampersand = '&',
  Mod = '%', Pipe = '|', Question = '?', Plus = '+', Minus = '-',

  // Special tokens
  End = '\0', None = 256, Number, String, Char, Ident,

  // Operators with multiple chars
  ShlAssign, ShrAssign,
  EqEq, NotEq,
  LessEq, GreaterEq,
  Shl, Shr,
  Inc, Dec,
  PlusAssign, MinusAssign,
  MulAssign, DivAssign,
  ModAssign,
};

extern const char *to_cstr(TokenType tt);
extern std::string_view op_text(TokenType t);
extern bool is_assign_op(TokenType t);

template <>
struct std::formatter<TokenType> : std::formatter<std::string> {
  auto format(TokenType tt, std::format_context& ctx) const
  {
    return std::formatter<std::string>::format(to_cstr(tt), ctx);
  }
};

struct Token {
  TokenType type;
  std::string_view lexeme;
  size_t row, col; // from start
};

class Lexer {
  const File &m_tu;
  std::string_view m_buf{};
  size_t m_row = 0, m_col = 0, m_cur = 0;

  void skip_comments_or_whitespaces();
  Token read_string(char indicator = '"');
  Token read_number();
  Token read_ident();

  char advance();
  char peek();

  template <typename... Args>
  void report_error(size_t row, size_t col, std::format_string<Args...> fmt, Args &&...args)
  {
    std::string msg = std::format(fmt, std::forward<Args...>(args)...);
    auto l = m_tu.line(row);
    eprintln("{}:{}:{}: error: {}", m_tu.name, row + 1, col, msg);
    eprintln("{:5} | {}", row + 1, l);
    eprintln("      | {:>{}}", "^", col);
  }

  template <typename... Args>
  void report_error(std::format_string<Args...> fmt, Args &&...args)
  { report_error(m_row, m_col, fmt, std::forward<Args...>(args)...); }

public:
  Lexer(const File &f) : m_tu(f), m_buf(f.content) {}

  Token next();
};
