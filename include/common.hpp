#pragma once
#include <variant>
#include <format>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define BASH_RED "\e[0;31m"
#define BASH_RESET "\e[0m"

#define TODOF(...) TODO(std::format(__VA_ARGS__))

#define TODO(msg) \
  do { \
    eprintln("{}:{}: TODO: {}", __FILE__, __LINE__, msg); \
    std::exit(31); \
  } while (0)

struct File {
  std::string name;
  std::string content;
  std::vector<size_t> line_starts;

  void index_lines() {
    line_starts.push_back(0);
    for (size_t i = 0; i < content.size(); i++)
      if (content[i] == '\n') line_starts.push_back(i + 1);
  }

  std::string_view line(size_t row) const {
    std::string_view c = content;
    size_t b = line_starts[row];
    size_t e = (row + 1 < line_starts.size()) ? line_starts[row + 1] : c.size();
    while (e > b && (c[e - 1] == '\n' || c[e - 1] == '\r')) e--;
    return c.substr(b, e - b);
  }
};

extern std::optional<File> read_entire_file(const char *file_name);

enum class TokenType {
  End = '\0', None = 1, Number, String, Character, Ident,
  OCurly = '{', CCurly = '}', OBracket = '[', CBracket = ']',
  OAngle = '<', CAngle = '>', OParen = '(', CParen = ')',
  Semicolon = ';', Plus = '+', Minus = '-', Asterisk = '*', Slash = '/',
  Comma = ',',
};

extern const char *to_cstr(TokenType tt);

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

struct DeferHelper {
  template <typename F>
  struct Guard {
    F f;
    ~Guard() { f(); }
  };
  template <typename F>
  Guard<F> operator +(F f) { return Guard<F>{std::move(f)}; }
};
#define _DEFER_CONCAT_(a, b) a##b
#define _DEFER_CONCAT(a, b) _DEFER_CONCAT_(a, b)
#define defer auto _DEFER_CONCAT(_defer_, __LINE__) = DeferHelper{} + [&]()

template <typename T, typename Variant>
struct is_alternative_of;

template <typename T, typename... Ts>
struct is_alternative_of<T, std::variant<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template <typename... Args>
inline void println(FILE *f, std::format_string<Args...> fmt, Args &&...args)
{
  auto s = std::format(fmt, std::forward<Args>(args)...);
  fprintf(f, "%.*s\n", (int)s.size(), s.c_str());
}

template <typename... Args>
inline void println(std::format_string<Args...> fmt, Args &&...args)
{
  println(stdout, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void eprintln(std::format_string<Args...> fmt, Args &&...args)
{
  println(stderr, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void print(FILE *f, std::format_string<Args...> fmt, Args &&...args)
{
  auto s = std::format(fmt, std::forward<Args>(args)...);
  fprintf(f, "%.*s", (int)s.size(), s.c_str());
}

template <typename... Args>
inline void print(std::format_string<Args...> fmt, Args &&...args)
{
  print(stdout, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void eprint(std::format_string<Args...> fmt, Args &&...args)
{
  print(stderr, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
[[noreturn]]
inline void panic(std::format_string<Args...> fmt, Args &&...args)
{
  eprint(BASH_RED "FATAL" BASH_RESET ": ");
  eprintln(fmt, std::forward<Args>(args)...);
  eprintln("compilation terminated.");
  std::exit(1);
}
