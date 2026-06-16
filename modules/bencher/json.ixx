// json.ixx
export module bencher.json;

#ifdef BENCHER_ENABLE_JSON

import std;

import bencher;

import glaze;

namespace bencher
{
   struct stage_result
   {
      std::string name{};
      std::vector<performance_metrics> results{};
   };

   export [[nodiscard]] inline std::string to_json(const stage& s)
   {
      stage_result output{s.name, s.results};
      return glz::write_json(output).value_or("{}");
   }

   export [[nodiscard]] inline std::string to_json_pretty(const stage& s)
   {
      stage_result output{s.name, s.results};
      return glz::write<glz::opts{.prettify = true}>(output).value_or("{}");
   }
}
#endif