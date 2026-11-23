#include "parser.hpp"

Expr *Parser::parse_program()
{
  std::vector<FunDef> functions;
  //std::vector<VarDef> globals;

  while (!expect(TokenType::End)) {
    if (m_tok.type == TokenType::Ident) {
      std::string_view name = m_tok.lexeme;
      consume();
      if (expect(TokenType::OParen)) functions.push_back(std::get<FunDef>(parse_function(name)->as));
      else if (expect(TokenType::OBracket)) TODO("global constant definition case");
      else report_error("expected expected '(' or '['");
    } else {
      report_error("expected an identifier");
      consume();
    }
  }

  return expr(Program{.functions=std::move(functions)});
}

bool Parser::expect(TokenType type)
{
  if (m_tok.type != type) return false;
  m_tok = m_l.next();
  return true;
}

void Parser::consume()
{ m_tok = m_l.next(); }

Expr *Parser::parse_statement()
{
  // { {statement}0 }
  if (expect(TokenType::OCurly)) {
    std::vector<Expr *> stmts;
    while (m_tok.type != TokenType::CCurly && m_tok.type != TokenType::End) {
      stmts.push_back(parse_statement());
    }
    if (!expect(TokenType::CCurly)) report_error("expected '}}'");
    return expr(Block{.stmts=std::move(stmts)});
  }

  if (m_tok.type == TokenType::Ident) {
    auto keyword = keyword_of(m_tok.lexeme);
    if (keyword != Keyword::None) {
      consume();
      switch (keyword) {
      case Keyword::Extrn: return parse_extrn();
      default: TODOF("Parse keyword for '{}'", keyword);
      }
    }
  }

  Expr *e = parse_rvalue();
  if (!expect(TokenType::Semicolon)) {
    report_error("expected ';'");
    consume();
  }
  return e;
}

// extrn name {, name}0 ; statement
Expr *Parser::parse_extrn()
{
  std::vector<Name> names;
  while (m_tok.type == TokenType::Ident) {
    names.push_back({m_tok.lexeme});
    consume();
    if (expect(TokenType::Comma)) continue;
    if (!expect(TokenType::Semicolon)) report_error("expected ';'");
    break;
  }
  return expr(ExtrnDecl{.names=std::move(names)});
}

Expr *Parser::parse_primary()
{
  // sub-expression case: (expr)
  if (expect(TokenType::OParen)) {
    Expr *e = parse_rvalue();
    if (!expect(TokenType::CParen)) report_error("expected ')'");
    return e;
  }

  defer { consume(); };
  switch (m_tok.type) {
  case TokenType::Ident: {
    std::string_view name = m_tok.lexeme;
    return expr(Name{name});
  }
  case TokenType::Character: {
    std::string escaped = escape_chars(m_tok);
    uint64_t value = 0;
    size_t n = escaped.size();
    if (n > 8) report_error(0, n, "too many characters ({}) in a literal maximum can be 8", n);
    else if (n == 0) report_error("empty character literals are not allowed");
    else value = pack_chars(escaped);
    return expr(Char{value});
  }
  case TokenType::Number: {
    auto sv = m_tok.lexeme;
    bool is_octal = *sv.data() == '0';
    int64_t value = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value, is_octal ? 8 : 10);
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
      report_error("invalid number");
    }
    return expr(Number{value});
  }
  case TokenType::String: {
    return expr(String{escape_chars(m_tok)});
  }
  default: {
    report_error("expected an expression");
    return nullptr;
  }
  }
}

Expr *Parser::parse_rvalue()
{
  Expr *e = parse_primary();

  // B allows calling constants like 'a'(); or 1();
  // So we don't constraint that primary expression must be identifier
  while (e && expect(TokenType::OParen)) {
    std::vector<Expr *> args;
    if (!expect(TokenType::CParen)) {
      while (1) {
        args.push_back(parse_rvalue());
        if (expect(TokenType::Comma)) continue;
        if (!expect(TokenType::CParen)) report_error("expected ',' or ')'");
        break;
      }
    }
    e = expr(FunCall{.callee=e, .args=std::move(args)});
  }
  return e;
}

Expr *Parser::parse_function(std::string_view name)
{
  // func_name(arg1) {}
  //           ^ -> lx_cur
  std::vector<Var> params{};
  while (!expect(TokenType::CParen)) {
    if (m_tok.type != TokenType::Ident) {
      report_error("expected an identifier in function parameters");
      break;
    }
    std::string_view param = m_tok.lexeme;
    consume();
    params.push_back({param});

    if (expect(TokenType::Comma)) continue;
    if (!expect(TokenType::CParen)) report_error("expected ',' or ')'");
    break;
  }

  Expr *body = nullptr;
  if (m_tok.type != TokenType::Semicolon && m_tok.type == TokenType::OCurly) {
    body = parse_statement();
  } else report_error("expected '{{'");
  return expr(FunDef{name, std::move(params), body});
}

// @Precondition: lit.type == TokenType::String || lit.type == TokenType::Character
std::string Parser::escape_chars(Token lit)
{
  std::string v;
  auto sv = lit.lexeme;

  for (auto it = sv.cbegin(); it != sv.cend(); it++) {
    if (*it == '*') {
      ++it;
      if (it == sv.cend()) {
        report_error("stray escape character");
        return v;
      }

      switch (*it) {
      case '0': v.push_back('\0'); break;
      case 'e': v.push_back('\x04'); break;
      case '(': v.push_back('{'); break;
      case ')': v.push_back('}'); break;
      case 't': v.push_back('\t'); break;
      case '*': v.push_back('*'); break;
      case '"': v.push_back('"'); break;
      case '\'': v.push_back('\''); break;
      case 'n': v.push_back('\n'); break;
      default: {
        report_error(it - sv.cbegin(), "invalid escape character: '*{}'", *it);
        return v;
      }
      }
    } else v.push_back(*it);
  }
  return v;
}

// @Precondition: 0 < sv.size() <= 8
uint64_t Parser::pack_chars(std::string_view sv)
{
  uint64_t v = 0;
  for (size_t i = 0; i < sv.size(); i++) {
    v |= (uint64_t)((unsigned char)sv[i]) << (8 * i);
  }
  return v;
}
