#pragma once
#include "lexer.hpp"
#include "common.hpp"
#include <vector>
#include <string_view>
#include <string>
#include <variant>
#include <cstdint>

enum class Keyword { None, Auto, Extrn, Goto, If, Else, While, Switch, Case, Return };

extern const char *to_cstr(Keyword keyword);
extern Keyword keyword_of(std::string_view s);

template <>
struct std::formatter<Keyword> : std::formatter<std::string> {
  auto format(Keyword keyword, std::format_context& ctx) const
  {
    return std::formatter<std::string>::format(to_cstr(keyword), ctx);
  }
};

extern int binop_prec(TokenType t);

struct Node;

struct Number { int64_t value; };
struct String { std::string value; };
struct Char   { uint64_t value; };
struct Name   { std::string_view name; };
struct BinOp  { TokenType op; Node *lhs; Node *rhs; };
struct IfStmt { Node *cond, *then_body, *else_body; };
struct Var    { std::string_view name; };
struct Block  { std::vector<Node *> stmts; };
struct FunDef {
  std::string_view name;
  std::vector<Var> params;
  Node *body;
};
struct ExtrnDecl { std::vector<Name> names; };
struct Program {
  std::vector<FunDef> functions;
};
struct FunCall {
  Node *callee;
  std::vector<Node *> args;
};
struct Null {};
struct IncDec {
  bool prefix;
  TokenType op;
  Node *value;
};

using NodeVariant = std::variant<
  Number, String, Char, Name, BinOp, FunCall,
  ExtrnDecl, IfStmt, Block, FunDef, Var, Null, Program,
  IncDec
>;

struct Node : NodeVariant {
  using variant::variant;
  using variant::operator=;
};

template <typename T>
concept NodeType = is_alternative_of<T, NodeVariant>::value;

template <NodeType T>
Node *node(T val)
{
  auto *node = new Node;
  *node = std::move(val);
  return node;
}

class Parser {
  Lexer &m_l;
  const File &m_tu;
  Token m_tok; // current token / cursor
  size_t m_nerr = 0;

  struct ReportErrorOpts {
    const Token *tok = nullptr;
    size_t off = 0, len = 1;
  };

  template <typename... Args>
  void report_error(ReportErrorOpts opts, std::format_string<Args...> fmt, Args &&...args)
  {
    std::string msg = std::format(fmt, std::forward<Args...>(args)...);
    const Token *tok = opts.tok ? opts.tok : &m_tok;
    auto l = m_tu.line(tok->row);

    if (m_nerr == 0) {
      eprintln("In file {}:", m_tu.name);
    }
    eprintln("{}:{}:{}: error: {}", m_tu.name, tok->row + 1, tok->col, msg);
    eprintln("{:5} | {}", tok->row + 1, l);
    eprintln("      | {}{}", std::string(tok->col + opts.off, ' '), std::string(std::max<size_t>(opts.len, 1), '^'));
    m_nerr++;
  }

  template <typename... Args>
  void report_error(std::format_string<Args...> fmt, Args &&...args)
  { report_error({}, fmt, std::forward<Args...>(args)...); }

  bool expect(TokenType type);
  void consume();
  bool iskey(Keyword keyword);

  std::string escape_chars(Token lit);
  uint64_t pack_chars(std::string_view sv);

  Node *parse_func(std::string_view name);
  Node *parse_stmt();

  Node *parse_primary();
  Node *parse_rvalue();
  Node *parse_postfix();
  Node *parse_ternary();
  Node *parse_assignment();
  Node *parse_unary();
  Node *parse_binary(int min_prec);

  Node *parse_extrn();
  Node *parse_ifstmt();
  Node *parse_while();
  Node *parse_goto();
  Node *parse_return();
  Node *parse_autodecl();
  Node *parse_switch();
  Node *parse_case();
public:
  Parser(Lexer &lexer, const File &f) : m_l(lexer), m_tu(f), m_tok(m_l.next()) {}

  Node *parse_program();
};
