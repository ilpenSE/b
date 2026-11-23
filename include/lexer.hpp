#pragma once
#include "common.hpp"

class Lexer {
  const File &m_tu;
  std::string_view m_buf{};
  size_t m_row = 0, m_col = 0, m_cur = 0;

  void skip_ws();
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
