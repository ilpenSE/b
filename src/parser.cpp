#include "parser.hpp"

Node *Parser::parse_program()
{
  std::vector<FunDef> functions;
  //std::vector<VarDef> globals;

  while (!expect(TokenType::End)) {
    if (m_tok.type == TokenType::Ident) {
      std::string_view name = m_tok.lexeme;
      consume();
      if (expect(TokenType::OParen)) functions.push_back(std::get<FunDef>(*parse_function(name)));
      else if (expect(TokenType::OBracket)) TODO("global constant definition case");
      else report_error("expected '(' or '['");
    } else {
      report_error("expected an identifier");
      consume();
    }
  }

  return node(Program{.functions=std::move(functions)});
}

Node *Parser::parse_statement()
{
  // null statement: ;
  if (expect(TokenType::Semicolon)) return node(Null{});

  // { {statement}0 }
  if (expect(TokenType::OCurly)) {
    std::vector<Node *> stmts;
    while (m_tok.type != TokenType::CCurly && m_tok.type != TokenType::End) {
      stmts.push_back(parse_statement());
    }
    if (!expect(TokenType::CCurly)) report_error("expected '}}'");
    return node(Block{.stmts=std::move(stmts)});
  }

  if (m_tok.type == TokenType::Ident) {
    auto keyword = keyword_of(m_tok.lexeme);
    if (keyword != Keyword::None) {
      switch (keyword) {
      case Keyword::Extrn: return parse_extrn();
      case Keyword::If: return parse_ifstmt();
      case Keyword::Else: {
        report_error("else without if");
        consume();
        // Simulate if that else has if and parse statement
        return parse_statement();
      }
      default: TODOF("Parse keyword for '{}'", keyword);
      }
    }
  }

  Node *e = parse_rvalue();
  if (!expect(TokenType::Semicolon)) {
    report_error("expected ';'");
    consume();
  }
  return e;
}

Node *Parser::parse_ifstmt()
{
  consume(); // if
  if (!expect(TokenType::OParen)) report_error("expected '('");
  Node *cond = parse_rvalue();
  if (!expect(TokenType::CParen)) report_error("expected ')'");
  Node *then_body = parse_statement();
  Node *else_body = nullptr;
  if (iskey(Keyword::Else)) {
    consume(); // else
    else_body = parse_statement();
  }
  return node(IfStmt{cond, then_body, else_body});
}

// extrn name {, name}0 ; statement
Node *Parser::parse_extrn()
{
  consume(); // extrn
  std::vector<Name> names;
  while (m_tok.type == TokenType::Ident) {
    names.push_back({m_tok.lexeme});
    consume();
    if (expect(TokenType::Comma)) continue;
    if (!expect(TokenType::Semicolon)) report_error("expected ';'");
    break;
  }
  return node(ExtrnDecl{.names=std::move(names)});
}

Node *Parser::parse_primary()
{
  // sub-nodeession case: (node)
  if (expect(TokenType::OParen)) {
    Node *e = parse_rvalue();
    if (!expect(TokenType::CParen)) report_error("expected ')'");
    return e;
  }

  defer { consume(); };
  switch (m_tok.type) {
  case TokenType::Ident: {
    std::string_view name = m_tok.lexeme;
    return node(Name{name});
  }
  case TokenType::Character: {
    std::string escaped = escape_chars(m_tok);
    uint64_t value = 0;
    size_t n = escaped.size();
    if (n > 8) report_error(0, n, "too many characters ({}) in a literal maximum can be 8", n);
    else if (n == 0) report_error("empty character literals are not allowed");
    else value = pack_chars(escaped);
    return node(Char{value});
  }
  case TokenType::Number: {
    auto sv = m_tok.lexeme;
    bool is_octal = *sv.data() == '0';
    int64_t value = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value, is_octal ? 8 : 10);
    if (ec != std::errc{} || ptr != sv.data() + sv.size()) {
      report_error("invalid number");
    }
    return node(Number{value});
  }
  case TokenType::String: {
    return node(String{escape_chars(m_tok)});
  }
  default: {
    report_error("expected an nodeession");
    return nullptr;
  }
  }
}

Node *Parser::parse_rvalue()
{
  Node *e = parse_primary();

  // B allows calling constants like 'a'(); or 1();
  // So we don't constraint that primary nodeession must be identifier
  while (e && expect(TokenType::OParen)) {
    std::vector<Node *> args;
    if (!expect(TokenType::CParen)) {
      while (1) {
        args.push_back(parse_rvalue());
        if (expect(TokenType::Comma)) continue;
        if (!expect(TokenType::CParen)) report_error("expected ',' or ')'");
        break;
      }
    }
    e = node(FunCall{.callee=e, .args=std::move(args)});
  }
  return e;
}

Node *Parser::parse_function(std::string_view name)
{
  std::vector<Var> params{};
  while (!expect(TokenType::CParen)) {
    if (m_tok.type != TokenType::Ident) {
      report_error("expected an identifier in function parameters");
      break;
    }
    params.push_back({m_tok.lexeme});
    consume();

    if (expect(TokenType::Comma)) continue;
    if (!expect(TokenType::CParen)) report_error("expected ',' or ')'");
    break;
  }

  Node *body = nullptr;
  if (m_tok.type != TokenType::Semicolon && m_tok.type == TokenType::OCurly) {
    body = parse_statement();
  } else report_error("expected '{{'");
  return node(FunDef{name, std::move(params), body});
}

// Helpers

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

bool Parser::expect(TokenType type)
{
  if (m_tok.type != type) return false;
  m_tok = m_l.next();
  return true;
}

bool Parser::iskey(Keyword keyword)
{
  return (m_tok.type == TokenType::Ident && keyword_of(m_tok.lexeme) == keyword);
}

void Parser::consume()
{ m_tok = m_l.next(); }
