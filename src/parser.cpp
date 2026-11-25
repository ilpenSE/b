#include "parser.hpp"
#include <unordered_map>

Node *Parser::parse_program()
{
  std::vector<FunDef> functions;
  //std::vector<VarDef> globals;

  while (!expect(TokenType::End)) {
    if (m_tok.type == TokenType::Ident) {
      std::string_view name = m_tok.lexeme;
      consume();
      if (expect(TokenType::OParen)) functions.push_back(std::get<FunDef>(*parse_func(name)));
      else if (expect(TokenType::OBracket)) todo("globals");
      else report_error("expected '(' or '['");
    } else {
      report_error("expected an identifier");
      consume();
    }
  }

  return node(Program{.functions=std::move(functions)});
}

Node *Parser::parse_stmt()
{
  // null statement: ;
  if (expect(TokenType::Semicolon)) return node(Null{});

  // { {statement}0 }
  if (expect(TokenType::OCurly)) {
    std::vector<Node *> stmts;
    while (m_tok.type != TokenType::CCurly && m_tok.type != TokenType::End) {
      stmts.push_back(parse_stmt());
    }
    if (!expect(TokenType::CCurly)) report_error("expected '}}'");
    return node(Block{.stmts=std::move(stmts)});
  }

  if (m_tok.type == TokenType::Ident) {
    auto keyword = keyword_of(m_tok.lexeme);
    if (keyword != Keyword::None) {
      switch (keyword) {
      case Keyword::If: return parse_ifstmt();
      case Keyword::Else: {
        report_error("else without if");
        consume();
        // Simulate if that else has if and parse statement
        return parse_stmt();
      }
      case Keyword::Extrn: return parse_extrn();
      case Keyword::While: return parse_while();
      case Keyword::Goto: return parse_goto();
      case Keyword::Return: return parse_return();
      case Keyword::Auto: return parse_autodecl();
      case Keyword::Switch: return parse_switch();
      case Keyword::Case: return parse_case();
      default: unreachable("keyword switch-case");
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

// auto name {constant}01 {, name {constant}01}0 ;  statement
Node *Parser::parse_autodecl()
{
  todo();
}

// switch rvalue statement
Node *Parser::parse_switch()
{
  todo();
}

// case constant : statement
Node *Parser::parse_case()
{
  todo();
}

// if ( rvalue ) statement {else statement}01
Node *Parser::parse_ifstmt()
{
  consume(); // if
  if (!expect(TokenType::OParen)) report_error("expected '('");
  Node *cond = parse_rvalue();
  if (!expect(TokenType::CParen)) report_error("expected ')'");
  Node *then_body = parse_stmt();
  Node *else_body = nullptr;
  if (iskey(Keyword::Else)) {
    consume(); // else
    else_body = parse_stmt();
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

// while ( rvalue ) statement
Node *Parser::parse_while()
{
  todo();
}

// goto rvalue ;
Node *Parser::parse_goto()
{
  todo();
}

// return {( rvalue )}01 ;
Node *Parser::parse_return()
{
  todo();
}

Node *Parser::parse_primary()
{
  // sub-expression case: (node)
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
  case TokenType::Char: {
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
    report_error("expected an expression");
    return nullptr;
  }
  }
}

Node *Parser::parse_rvalue()
{
  return parse_assignment();
}

Node *Parser::parse_binary(int min_prec)
{
  Node *lhs = parse_unary();
  while (true) {
    int prec = binop_prec(m_tok.type);
    if (prec < min_prec) break;
    TokenType op = m_tok.type;
    consume();
    Node *rhs = parse_binary(prec + 1);
    if (!rhs) return lhs;
    lhs = node(BinOp{op, lhs, rhs});
  }
  return lhs;
}

Node *Parser::parse_unary()
{
  return parse_postfix();
}

Node *Parser::parse_ternary()
{ return parse_binary(0); }

Node *Parser::parse_assignment()
{ return parse_ternary(); }

Node *Parser::parse_postfix()
{
  // B allows calling constants like 'a'(); or 1();
  // So we don't constraint that primary expression must be identifier
  Node *e = parse_primary();
  while (e) {
    if (expect(TokenType::OParen)) {
      std::vector<Node *> args;
      if (!expect(TokenType::CParen)) {
        while (true) {
          args.push_back(parse_rvalue());
          if (expect(TokenType::Comma)) continue;
          if (!expect(TokenType::CParen)) report_error("expected ',' or ')'");
          break;
        }
      }
      e = node(FunCall{.callee=e, .args=std::move(args)});
    } else if (m_tok.type == TokenType::Inc || m_tok.type == TokenType::Dec) {
      TokenType op = m_tok.type; consume();
      e = node(PostIncDec{op, e});
    } else break;
  }
  return e;
}

Node *Parser::parse_func(std::string_view name)
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
    body = parse_stmt();
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

int binop_prec(TokenType t)
{
  switch (t) {
  case TokenType::Asterisk:
  case TokenType::Slash:
  case TokenType::Mod:     return 10;

  case TokenType::Plus:
  case TokenType::Minus:       return 9;

  case TokenType::Shl:
  case TokenType::Shr:         return 8;

  case TokenType::LessThan:
  case TokenType::LessEq:
  case TokenType::GreaterThan:
  case TokenType::GreaterEq:   return 7;

  case TokenType::EqEq:
  case TokenType::NotEq:       return 6;

  case TokenType::Ampersand:   return 5;
  case TokenType::Pipe:        return 4;

  default:                     return -1;
  }
}

const char *to_cstr(Keyword keyword)
{
  switch (keyword) {
  case Keyword::Auto: return "auto";
  case Keyword::Extrn: return "extrn";
  case Keyword::Goto: return "goto";
  case Keyword::If: return "if";
  case Keyword::Else: return "else";
  case Keyword::While: return "while";
  case Keyword::Switch: return "switch";
  case Keyword::Case: return "case";
  case Keyword::Return: return "return";
  default: return "<invalid>";
  }
}

Keyword keyword_of(std::string_view s) {
  static const std::unordered_map<std::string_view, Keyword> table = {
    {"auto", Keyword::Auto},   {"extrn", Keyword::Extrn},
    {"goto", Keyword::Goto},   {"if", Keyword::If},
    {"else", Keyword::Else},   {"while", Keyword::While},
    {"switch", Keyword::Switch}, {"case", Keyword::Case},
    {"return", Keyword::Return},
  };
  auto it = table.find(s);
  return it == table.end() ? Keyword::None : it->second;
}
