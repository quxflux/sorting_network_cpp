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

template<typename T, std::size_t N, typename F>
auto benchmark_sorting_function(const F& f)
{
  auto data_to_sort = generate_benchmark_data<T, N>();

  duration_t sum{};

  for (std::size_t i = 0; i < data_to_sort.size(); ++i)
  {
    sum += measure_execution_time<duration_t>(f, data_to_sort[i].begin());
  }

  return sum;
}

template<typename ValueType, std::size_t N, quxflux::sorting_net::type type>
struct sorting_network_benchmark
{
  benchmark_result operator()() const
  {
    benchmark_result result{to_string<ValueType>(), N, to_string<type>()};

    if constexpr (quxflux::sorting_net::available_v<N, type>)
    {
      std::clog << to_string<ValueType>() << ", " << N << " item(s), " << to_string<type>() << '\n';

      if constexpr (is_msvc && type == quxflux::sorting_net::type::insertion_sort && N >= 32)
      {
        // msvc is having trouble to generate the insertion sort network
        return result;
      } else
      {
        const auto duration = benchmark_sorting_function<ValueType, N>(
          [](const auto it) { quxflux::sorting_net::sorting_network<N, type>{}(it); });

        result.avg_exec_time = duration;
      }
    }

    return result;
  }
};

template<typename ValueType, std::size_t N>
struct std_sort_benchmark
{
  benchmark_result operator()() const
  {
    std::clog << to_string<ValueType>() << ", " << N << " item(s), std::sort\n";

    const auto duration = benchmark_sorting_function<ValueType, N>([](const auto it) { std::sort(it, it + N); });

    return benchmark_result{to_string<ValueType>(), N, "std::sort", duration};
  }
};

template<std::size_t N, typename ValueType>
void benchmark_all_with_size_and_type(std::set<benchmark_result>& benchmark_results)
{
  using network_type = quxflux::sorting_net::type;

  benchmark_results.insert(sorting_network_benchmark<ValueType, N, network_type::batcher_odd_even_merge_sort>{}());
  benchmark_results.insert(sorting_network_benchmark<ValueType, N, network_type::bitonic_merge_sort>{}());
  benchmark_results.insert(sorting_network_benchmark<ValueType, N, network_type::bose_nelson_sort>{}());
  benchmark_results.insert(sorting_network_benchmark<ValueType, N, network_type::bubble_sort>{}());
  benchmark_results.insert(sorting_network_benchmark<ValueType, N, network_type::insertion_sort>{}());
  benchmark_results.insert(sorting_network_benchmark<ValueType, N, network_type::size_optimized_sort>{}());
  benchmark_results.insert(std_sort_benchmark<ValueType, N>{}());

  std::clog << '\n';
}

template<std::size_t N>
void benchmark_all_with_size(std::set<benchmark_result>& benchmark_results)
{
  benchmark_all_with_size_and_type<N, std::int16_t>(benchmark_results);
  benchmark_all_with_size_and_type<N, std::int32_t>(benchmark_results);
  benchmark_all_with_size_and_type<N, std::uint32_t>(benchmark_results);
  benchmark_all_with_size_and_type<N, std::int64_t>(benchmark_results);
  benchmark_all_with_size_and_type<N, float>(benchmark_results);
  benchmark_all_with_size_and_type<N, double>(benchmark_results);
  benchmark_all_with_size_and_type<N, Vec2i>(benchmark_results);

  std::clog << '\n';
}

int main(int, const char**)
{
  std::set<benchmark_result> benchmark_results;

  benchmark_all_with_size<1>(benchmark_results);
  benchmark_all_with_size<2>(benchmark_results);
  benchmark_all_with_size<4>(benchmark_results);
  benchmark_all_with_size<8>(benchmark_results);
  benchmark_all_with_size<16>(benchmark_results);
  benchmark_all_with_size<32>(benchmark_results);
  benchmark_all_with_size<64>(benchmark_results);
  benchmark_all_with_size<128>(benchmark_results);

  std::set<std::string_view> all_algorithm_names;

  for (const auto& benchmark_result : benchmark_results)
    all_algorithm_names.insert(benchmark_result.algorithm);

  benchmark_table table;

  for (const auto& benchmark_result : benchmark_results)
  {
    table[benchmark_scenario{benchmark_result.data_type, benchmark_result.n}][benchmark_result.algorithm] =
      benchmark_result.avg_exec_time;
  }

  std::ofstream ofs("timings.csv");
  ofs << table;

  return EXIT_SUCCESS;
}
