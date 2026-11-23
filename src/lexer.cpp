#include "lexer.hpp"
#include <cctype>

void Lexer::skip_ws()
{
  while (isspace((unsigned char)peek())) advance();
}

Token Lexer::read_string(char indicator)
{
  size_t old_row = m_row, old_col = m_col, old_cur = m_cur;
  while (peek() != indicator) advance();
  Token r = {
    .type=(indicator == '"' ? TokenType::String : TokenType::Character),
    .lexeme=m_buf.substr(old_cur, m_cur - old_cur),
    .row=old_row, .col=old_col,
  };
  advance();
  return r;
}

Token Lexer::read_number()
{
  size_t old_row = m_row, old_col = m_col, old_cur = m_cur;
  while (std::isdigit((unsigned char)peek())) advance();
  return Token{
    .type=TokenType::Number,
    .lexeme=m_buf.substr(old_cur, m_cur - old_cur),
    .row=old_row, .col=old_col,
  };
}

Token Lexer::read_ident()
{
  size_t old_row = m_row, old_col = m_col, old_cur = m_cur;
  while (std::isalnum((unsigned char)peek())) advance();
  return Token{
    .type=TokenType::Ident,
    .lexeme=m_buf.substr(old_cur, m_cur - old_cur),
    .row=old_row, .col=old_col,
  };
}

Token Lexer::next()
{
again:
  skip_ws();
  char c = peek();
  switch (c) {
  case '{': case '}': case '[': case ']':
  case '(': case ')': case '<': case '>':
  case ';': case '\0': case '+': case '-':
  case '*': {
    Token t = {
      .type=static_cast<TokenType>(c), .lexeme="",
      .row=m_row, .col=m_col
    };
    advance();
    return t;
  }
  case '/': {
    if (m_cur + 1 < m_buf.size() && m_buf[m_cur + 1] == '*') { // comment case
      size_t start_row = m_row, start_col = m_col;
      advance(); advance(); // skip '/*'
      while (1) {
        if (m_cur >= m_buf.size()) {
          report_error(start_row, start_col, "unclosed comment");
          break;
        }
        if (m_buf[m_cur] == '*'
          && m_cur + 1 < m_buf.size()
          && m_buf[m_cur + 1] == '/') {
          advance(); advance(); // skip '*/'
          break;
        }
        advance();
      }
      goto again;
    } else {
      defer { advance(); };
      return Token{.type=TokenType::Slash, .lexeme="", .row=m_row, .col=m_col};
    }
  } break;
  }

  if ((c >= '0' && c <= '9') ||
     c == '-') {
    return read_number();
  }

  if (c == '"' || c == '\'') {
    advance(); return read_string(c);
  }

  // Identifiers should contain ASCII and _ meaning that
  // no support for non-ASCII chars like special Turkish characters, Cyrillic/Indian/Japanese/Chinese alphabets etc.
  if (std::isalpha((unsigned char)c) || c == '_') return read_ident();

  // invalid char / unexpected symbol
  Token t = {.type=TokenType::None, .lexeme=m_buf.substr(m_cur, 1),
             .row=m_row, .col=m_col};
  advance();
  report_error("unexpected characeter '{}'", t.lexeme);
  return t;
}

char Lexer::advance()
{
  if (m_cur >= m_buf.size()) return '\0';
  char c = m_buf[m_cur++];
  if (c == '\n') (m_row++, m_col = 0); else m_col++;
  return c;
}

char Lexer::peek()
{
  if (m_cur >= m_buf.size()) return '\0';
  return m_buf[m_cur];
}
