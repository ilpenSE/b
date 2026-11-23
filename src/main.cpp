#include "common.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <string_view>
#include <vector>
#include <climits>

void print_usage(std::string_view program_name, FILE *f)
{
  println(f, "Usage: {} <...input file> [...OPTIONS]", program_name);
  println(f, "Options:");
  println(f, "    --help | -h [command]: Print this help message or get help about a command.");
}

const char *shift(int &argc, char **&argv)
{ return argc == 0 ? nullptr : (argc--, *argv++); }

// Dumps AST
void dump(const Expr *e, int depth = 0);

int main(int argc, char **argv)
{
  std::string_view program_name = shift(argc, argv);
  std::vector<File> input_files;

  while (const char *arg_cstr = shift(argc, argv)) {
    std::string_view arg = arg_cstr;
    if (*arg.data() == '-') {
      if (arg == "--help" || arg == "-h") {
        print_usage(program_name, stdout);
        return 0;
      } else {
        panic("unrecognized option: {}", arg);
      }
    } else {
      auto fres = read_entire_file(arg_cstr);
      if (fres) {
        input_files.push_back(std::move(*fres));
      } else panic("could not find file: {}", arg);
    }
  }

  static_assert(sizeof(void *) == 8, "Word size should be 8 bytes (64 bits)");

  for (const File &f : input_files) {
    Lexer l(f);
    Parser p(l, f);

    Expr *program = p.parse_program();
    dump(program);
  }
}

static std::string join_params(const std::vector<Var> &ps) {
  std::string out;
  for (size_t i = 0; i < ps.size(); i++) {
    if (i) out += ", ";
    out += ps[i].name;
  }
  return out;
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

static void indent(int depth) {
  for (int i = 0; i < depth; i++) print("  ");
}

void dump(const Expr *e, int depth)
{
  indent(depth);
  if (!e) { println("<null>"); return; }

  std::visit(overloaded{
    [&](const Program &p) {
      println("Program");
      for (const FunDef &f : p.functions) {
        indent(depth + 1);
        println("FunDef {}({})", f.name, join_params(f.params));
        dump(f.body, depth + 2);
      }
    },
    [&](const Block &b) {
      println("Block");
      for (const Expr *s : b.stmts) dump(s, depth + 1);
    },
    [&](const ExtrnDecl &x) {
      print("Extrn");
      for (const Name &n : x.names) print(" {}", n.name);
      println("");
    },
    [&](const FunCall &c) {
      println("Call");
      indent(depth + 1); println("callee:");
      dump(c.callee, depth + 2);
      indent(depth + 1); println("args:");
      for (const Expr *a : c.args) dump(a, depth + 2);
    },
    [&](const BinOp &b) {
      println("BinOp '{}'", b.op);
      dump(b.lhs, depth + 1);
      dump(b.rhs, depth + 1);
    },
    [&](const IfStmt &i) {
      println("If");
      dump(i.cond, depth + 1);
      dump(i.then_body, depth + 1);
      if (i.else_body) dump(i.else_body, depth + 1);
    },
    [&](const Name &n)   { println("Name {}", n.name); },
    [&](const Var &v)    { println("Var {}", v.name); },
    [&](const Number &n) { println("Number {}", n.value); },
    [&](const Char &c)   { println("Char 0x{:x}", c.value); },
    [&](const String &s) { println("String \"{}\"", s.value); },
    [&](const FunDef &f) {
      println("FunDef {}({})", f.name, join_params(f.params));
      dump(f.body, depth + 1);
    },
  }, e->as);
}
