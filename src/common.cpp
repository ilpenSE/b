#include "common.hpp"
#include <unordered_map>

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
