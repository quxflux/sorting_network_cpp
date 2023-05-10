// This file is part of the sorting_network_cpp (https://github.com/quxflux/sorting_network_cpp).
// Copyright (c) 2022 Lukas Riebel.
//
// sorting_network_cpp is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// sorting_network_cpp is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <array>
#include <fstream>
#include <iostream>
#include <set>

#include <sorting_network_cpp/sorting_network.h>

#include "util.h"

namespace quxflux
{
  namespace
  {
    const std::string compiler_id = []() {
      std::ostringstream os;

#if defined(_MSC_VER)
      os << "msvc " << _MSC_VER;
#elif defined(__clang__)
      os << "clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__;
#elif defined(__GNUC__)
      os << "gcc " << __GNUC__ << "." << __GNUC_MINOR__;
#else
      os << "unknown compiler";
#endif

      return os.str();
    }();

    struct benchmark_scenario
    {
      std::string_view data_type;
      std::size_t n = 0;

      constexpr bool operator==(const benchmark_scenario& rhs) const
      {
        return data_type == rhs.data_type && n == rhs.n;
      }

      constexpr bool operator<(const benchmark_scenario& rhs) const
      {
        return std::tie(data_type, n) < std::tie(rhs.data_type, rhs.n);
      }
    };

    using benchmark_table = std::map<benchmark_scenario, std::map<std::string_view, std::optional<duration_t>>>;

    std::ostream& operator<<(std::ostream& os, const benchmark_table& table)
    {
      os << "compiler\tdata type\tN\t";

      if (table.empty())
        return os;

      for ([[maybe_unused]] const auto& [algorithm, duration] : table.begin()->second)
      {
        os << algorithm << '\t';
      }

      os << '\n';

      for (const auto& scenario : table)
      {
        os << compiler_id << '\t' << scenario.first.data_type << '\t' << scenario.first.n << '\t';

        for (const auto [algorithm, duration] : scenario.second)
        {
          if (duration.has_value())
            os << duration->count();
          else
            os << ' ';

          os << '\t';
        }

        os << '\n';
      }

      return os;
    }
  }  // namespace
}  // namespace quxflux

int main(int, const char**)
{
  namespace qf = quxflux;

  std::set<qf::benchmark_result> benchmark_results;

  qf::run_benchmark<int16_t>(benchmark_results);
  qf::run_benchmark<int32_t>(benchmark_results);
  qf::run_benchmark<uint32_t>(benchmark_results);
  qf::run_benchmark<int64_t>(benchmark_results);
  qf::run_benchmark<float>(benchmark_results);
  qf::run_benchmark<double>(benchmark_results);
  qf::run_benchmark<qf::vec2i>(benchmark_results);

  std::set<std::string_view> all_algorithm_names;

  for (const auto& benchmark_result : benchmark_results)
    all_algorithm_names.insert(benchmark_result.algorithm);

  qf::benchmark_table table;

  for (const auto& benchmark_result : benchmark_results)
  {
    table[qf::benchmark_scenario{benchmark_result.data_type, benchmark_result.n}][benchmark_result.algorithm] =
      benchmark_result.avg_exec_time;
  }

  std::ofstream ofs("timings.csv");
  ofs << table;

  return EXIT_SUCCESS;
}
