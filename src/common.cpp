#include "common.hpp"
#include <unordered_map>

const char *to_cstr(TokenType tt)
{
  switch (tt) {
  case TokenType::End: return "End";
  case TokenType::Number: return "Number";
  case TokenType::String: return "String";
  case TokenType::Character: return "Character";
  case TokenType::Ident: return "Ident";
  case TokenType::OCurly: return "OCurly";
  case TokenType::CCurly: return "CCurly";
  case TokenType::OBracket: return "OBracket";
  case TokenType::CBracket: return "CBracket";
  case TokenType::OAngle: return "OAngle";
  case TokenType::CAngle: return "CAngle";
  case TokenType::OParen: return "OParen";
  case TokenType::CParen: return "CParen";
  case TokenType::Semicolon: return "Semicolon";
  default: return "<invalid>";
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

std::optional<File> read_entire_file(const char *file_name)
{
  FILE *f = fopen(file_name, "rb");
  if (!f) return std::nullopt;
  defer { fclose(f); };

  if (fseek(f, 0, SEEK_END) != 0) return std::nullopt;
  long lsize = ftell(f);
  if (lsize < 0) return std::nullopt;
  size_t size = (size_t)lsize;
  if (fseek(f, 0, SEEK_SET) != 0) return std::nullopt;

  std::string buffer(size + 1, '\0');
  size_t n = fread(buffer.data(), 1, size, f);
  if (n != size) return std::nullopt;
  File ret = {.name=file_name, .content=std::move(buffer)};
  ret.index_lines();
  return ret;
}
