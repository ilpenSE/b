#pragma once
#include <variant>
#include <format>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <source_location>

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

[[noreturn]]
inline void todo(std::string_view msg = "", std::source_location loc = std::source_location::current())
{
  eprintln("{}:{}:{}: TODO: {}{}{}", loc.file_name(), loc.line(), loc.column(), loc.function_name(), (msg == "" ? "" : ": "), msg);
  std::exit(31);
}

[[noreturn]]
inline void unreachable(std::string_view msg = "", std::source_location loc = std::source_location::current())
{
  eprintln("{}:{}:{}: UNREACHABLE: {}{}{}", loc.file_name(), loc.line(), loc.column(), loc.function_name(), (msg == "" ? "" : ": "), msg);
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
  __assume(false);
#else // GNUC, clang, etc.
  __builtin_unreachable();
#endif
}

#define BASH_RED "\e[0;31m"
#define BASH_RESET "\e[0m"

template <typename... Args>
[[noreturn]]
inline void panic(std::format_string<Args...> fmt, Args &&...args)
{
  eprint(BASH_RED "FATAL" BASH_RESET ": ");
  eprintln(fmt, std::forward<Args>(args)...);
  eprintln("compilation terminated.");
  std::exit(1);
}
