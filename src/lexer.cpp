#include "lexer.hpp"
#include <cctype>

namespace {
constexpr const Token ops[] = {
  {TokenType::ShlAssign, "<<="}, {TokenType::ShrAssign, ">>="},

  {TokenType::EqEq, "=="},       {TokenType::NotEq, "!="},
  {TokenType::LessEq, "<="},     {TokenType::GreaterEq, ">="},
  {TokenType::Shl, "<<"},        {TokenType::Shr, ">>"},
  {TokenType::Inc, "++"},        {TokenType::Dec, "--"},
  {TokenType::PlusAssign, "+="}, {TokenType::MinusAssign, "-="},
  {TokenType::MulAssign, "*="},  {TokenType::DivAssign, "/="},
  {TokenType::ModAssign, "%="},

  {TokenType::OCurly, "{"},    {TokenType::CCurly, "}"},
  {TokenType::OBracket, "["},  {TokenType::CBracket, "]"},
  {TokenType::OParen, "("},    {TokenType::CParen, ")"},
  {TokenType::LessThan, "<"},  {TokenType::GreaterThan, ">"},
  {TokenType::Mod, "%"},       {TokenType::Pipe, "|"},
  {TokenType::Colon, ":"},     {TokenType::Question, "?"},
  {TokenType::Asterisk, "*"},  {TokenType::Ampersand, "&"},
  {TokenType::Semicolon, ";"}, {TokenType::Comma, ","},
  {TokenType::Eq, "="},        {TokenType::Plus, "+"},
  {TokenType::Minus, "-"},     {TokenType::Not, "!"},
};

constexpr auto make_ops_table()
{
  std::array<std::string_view, 1024> t{};
  for (const auto &op : ops) t[static_cast<size_t>(op.type)] = op.lexeme;
  return t;
}

constexpr auto ops_table = make_ops_table();
} // namespace

bool is_assign_op(TokenType t)
{
  switch (t) {
  case TokenType::Eq:
  case TokenType::PlusAssign:
  case TokenType::MinusAssign:
  case TokenType::MulAssign:
  case TokenType::DivAssign:
  case TokenType::ModAssign:
  case TokenType::ShlAssign:
  case TokenType::ShrAssign:
    return true;
  default:
    return false;
  }
}

std::string_view op_text(TokenType t)
{ return ops_table[static_cast<size_t>(t)]; }

Token Lexer::read_string(char indicator)
{
  size_t old_row = m_row, old_col = m_col, old_cur = m_cur;
  while (peek() != indicator) {
    // blindly skip any char after escape char
    if (peek() == '*') advance();
    advance();
  }

  defer { advance(); };
  return Token{
    .type=(indicator == '"' ? TokenType::String : TokenType::Char),
    .lexeme=m_buf.substr(old_cur, m_cur - old_cur),
    .row=old_row, .col=old_col,
  };
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

void Lexer::skip_comments_or_whitespaces()
{
  while (true) {
    while (isspace((unsigned char)peek())) advance();
    char c = peek();
    if (c == '/' && m_cur + 1 < m_buf.size() && m_buf[m_cur + 1] == '*') {
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
        advance(); // skip char in comment
      }
      continue;
    }
    break;
  }
}

Token Lexer::next()
{
  skip_comments_or_whitespaces();

  char c = peek();

  if (c >= '0' && c <= '9') {
    return read_number();
  }

  if (c == '"' || c == '\'') {
    advance(); return read_string(c);
  }

  // Identifiers should contain ASCII and _ meaning that
  // no support for non-ASCII chars like special Turkish characters, Cyrillic/Indian/Japanese/Chinese alphabets etc.
  if (std::isalpha((unsigned char)c) || c == '_') return read_ident();

  // operator / symbol
  std::string_view rest = m_buf.substr(m_cur);
  for (auto &op : ops) {
    if (rest.starts_with(op.lexeme)) {
      Token t = {.type=op.type, .row=m_row, .col=m_col};
      for (size_t i = 0; i < op.lexeme.size(); i++) advance();
      return t;
    }
  }

  if (peek() == '\0') return Token{.type=TokenType::End, .row=m_row, .col=m_col};

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

const char *to_cstr(TokenType tt)
{
  switch (tt) {
  case TokenType::End: return "End";
  case TokenType::Number: return "Number";
  case TokenType::String: return "String";
  case TokenType::Char: return "Char";
  case TokenType::Ident: return "Ident";
  case TokenType::OCurly: return "OCurly";
  case TokenType::CCurly: return "CCurly";
  case TokenType::OBracket: return "OBracket";
  case TokenType::CBracket: return "CBracket";
  case TokenType::GreaterThan: return "GreaterThan";
  case TokenType::LessThan: return "LesserThan";
  case TokenType::OParen: return "OParen";
  case TokenType::CParen: return "CParen";
  case TokenType::Semicolon: return "Semicolon";
  case TokenType::Colon: return "Colon";
  case TokenType::Eq: return "Eq";
  case TokenType::Plus: return "Plus";
  case TokenType::Minus: return "Minus";
  case TokenType::Asterisk: return "Asterisk";
  case TokenType::Slash: return "Slash";
  case TokenType::Comma: return "Comma";
  case TokenType::Ampersand: return "Ampersand";
  case TokenType::Mod: return "Mod";
  case TokenType::Pipe: return "Pipe";
  case TokenType::Question: return "Question";
  case TokenType::None: return "None";
  case TokenType::ShlAssign: return "ShlAssign";
  case TokenType::ShrAssign: return "ShrAssign";
  case TokenType::EqEq: return "EqEq";
  case TokenType::NotEq: return "NotEq";
  case TokenType::LessEq: return "LessEq";
  case TokenType::GreaterEq: return "GreaterEq";
  case TokenType::Shl: return "Shl";
  case TokenType::Shr: return "Shr";
  case TokenType::Inc: return "Inc";
  case TokenType::Dec: return "Dec";
  case TokenType::PlusAssign: return "PlusAssign";
  case TokenType::MinusAssign: return "MinusAssign";
  case TokenType::MulAssign: return "MulAssign";
  case TokenType::DivAssign: return "DivAssign";
  case TokenType::ModAssign: return "ModAssign";
  default: return "<invalid>";
  }
}
