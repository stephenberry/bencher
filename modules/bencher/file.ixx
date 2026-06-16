// file.ixx
export module bencher.file;

import std;

namespace bencher
{
   export inline bool save_file(const std::string& contents, const std::string& path)
   {
      std::ofstream file(path.data());
      if (not file) {
         return false;
      }
      file.write(contents.data(), static_cast<std::int64_t>(contents.size()));
      return true;
   }
}
