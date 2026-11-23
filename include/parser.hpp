#pragma once
#include "lexer.hpp"
#include "common.hpp"
#include <vector>
#include <string_view>
#include <string>
#include <variant>
#include <cstdint>

// later
// enum class Operator {
//   AssignPlus = 0 /* += */, AssignMinus /* -= */, AssignMult /* *= */, AssignDiv /* /= */,
//   Plus = '+', Minus = '-', Asterisk = '*', Slash = '/',
// };

struct Expr;

struct Number { int64_t value; };
struct String { std::string value; };
struct Char   { uint64_t value; };
struct Name   { std::string_view name; };
struct BinOp  { char op; Expr *lhs; Expr *rhs; };
struct IfStmt { Expr *cond, *then_body, *else_body; };
struct Var    { std::string_view name; };
struct Block  { std::vector<Expr *> stmts; };
struct FunDef {
  std::string_view name;
  std::vector<Var> params;
  Expr *body;
};
struct ExtrnDecl { std::vector<Name> names; };
struct Program {
  std::vector<FunDef> functions;
};
struct FunCall {
  Expr *callee;
  std::vector<Expr *> args;
};

struct Expr {
  std::variant<Number, String, Char, Name, BinOp, FunCall,
    ExtrnDecl, IfStmt, Block, FunDef, Var, Program> as;
};

using ExprVariant = decltype(Expr::as);

template <typename T>
concept ExprNode = is_alternative_of<T, ExprVariant>::value;

template <ExprNode T>
Expr *expr(T val)
{
  auto *e = new Expr;
  e->as = std::move(val);
  return e;
}

class Parser {
  Lexer &m_l;
  const File &m_tu;
  Token m_tok; // current token / cursor

  template <typename... Args>
  void report_error(size_t off, size_t len, std::format_string<Args...> fmt, Args &&...args)
  {
    std::string msg = std::format(fmt, std::forward<Args...>(args)...);
    auto l = m_tu.line(m_tok.row);
    eprintln("{}:{}:{}: error: {}", m_tu.name, m_tok.row + 1, m_tok.col, msg);
    eprintln("{:5} | {}", m_tok.row + 1, l);
    eprintln("      | {}{}", std::string(m_tok.col + off, ' '), std::string(std::max<size_t>(len, 1), '^'));
  }

  template <typename... Args>
  void report_error(std::format_string<Args...> fmt, Args &&...args)
  { report_error(0, 1, fmt, std::forward<Args...>(args)...); }

  template <typename... Args>
  void report_error(size_t off, std::format_string<Args...> fmt, Args &&...args)
  { report_error(off, 1, fmt, std::forward<Args...>(args)...); }

  bool expect(TokenType type);
  void consume();

  std::string escape_chars(Token lit);
  uint64_t pack_chars(std::string_view sv);

  Expr *parse_function(std::string_view name);
  Expr *parse_statement();
  Expr *parse_rvalue();
  Expr *parse_primary();
  Expr *parse_extrn();
public:
  Parser(Lexer &lexer, const File &f) : m_l(lexer), m_tu(f), m_tok(m_l.next()) {}

  Expr *parse_program();
};
