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

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>

#include <sorting_network_cpp/sorting_network.h>

namespace quxflux
{
  struct vec2i
  {
    std::array<uint16_t, 2> data;
  };

  using duration_t = std::chrono::duration<double, std::milli>;

  struct benchmark_result
  {
    std::string_view data_type;
    std::size_t n = 0;
    std::string_view algorithm;

    std::optional<duration_t> avg_exec_time;

    constexpr bool operator<(const benchmark_result& rhs) const
    {
      return std::tie(data_type, n, algorithm) < std::tie(rhs.data_type, rhs.n, rhs.algorithm);
    }
  };

  namespace detail
  {
    template<typename T, std::size_t N>
    std::vector<std::array<T, N>> generate_benchmark_data()
    {
      std::vector<std::array<T, N>> unsorted_data(1'000'000);
      std::mt19937 rd(42);

      for (auto& array : unsorted_data)
      {
        std::generate(array.begin(), array.end(), [&]() -> T {
          if constexpr (std::is_same_v<vec2i, T>)
          {
            std::uniform_int_distribution<uint16_t> dis{};

            return {dis(rd), dis(rd)};
          } else
          {
            if constexpr (std::is_floating_point_v<T>)
            {
              return std::uniform_real_distribution<T>{}(rd);
            } else
            {
              return static_cast<T>(
                std::uniform_int_distribution<std::size_t>{0, std::size_t{std::numeric_limits<T>::max()}}(rd));
            }
          }
        });
      }

      return unsorted_data;
    }

    template<typename Duration, typename F, typename... Args>
    Duration measure_execution_time(const F& f, Args&&... args)
    {
      using clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
                                       std::chrono::high_resolution_clock, std::chrono::steady_clock>;

      const auto start = clock::now();
      f(std::forward<Args>(args)...);
      const auto end = clock::now();

      return std::chrono::duration_cast<Duration>(end - start);
    }

    template<typename T>
    constexpr std::string_view to_string()
    {
      const std::unordered_map<std::type_index, std::string_view> name_map{
        std::pair{std::type_index(typeid(int16_t)), "int16_t"},      //
        std::pair{std::type_index(typeid(int32_t)), "int32_t"},      //
        std::pair{std::type_index(typeid(int64_t)), "int64_t"},      //
        std::pair{std::type_index(typeid(uint32_t)), "uint32_t"},    //
        std::pair{std::type_index(typeid(float)), "float"},          //
        std::pair{std::type_index(typeid(double)), "double"},        //
        std::pair{std::type_index(typeid(vec2i)), "vec2i Z-order"},  //
      };

      const auto it = name_map.find(std::type_index{typeid(T)});
      if (it == name_map.end())
        throw std::invalid_argument("type not present in name map");

      return it->second;
    }

    template<quxflux::sorting_net::type NWT>
    constexpr std::string_view to_string()
    {
      using SN = quxflux::sorting_net::type;

      const std::unordered_map<quxflux::sorting_net::type, std::string_view> name_map{
        std::pair{SN::bubble_sort, "SN::bubble_sort"},                                  //
        std::pair{SN::insertion_sort, "SN::insertion_sort"},                            //
        std::pair{SN::batcher_odd_even_merge_sort, "SN::batcher_odd_even_merge_sort"},  //
        std::pair{SN::bitonic_merge_sort, "SN::bitonic_merge_sort"},                    //
        std::pair{SN::size_optimized_sort, "SN::size_optimized_sort"},                  //
        std::pair{SN::bose_nelson_sort, "SN::bose_nelson_sort"},                        //
      };

      const auto it = name_map.find(NWT);
      if (it == name_map.end())
        throw std::invalid_argument("type not present in name map");

      return it->second;
    }

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

          static constexpr bool is_msvc =
#if defined(_MSC_VER)
            true
#else
            false
#endif
            ;

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

    template<typename T>
    struct benchmark_impl
    {
      void operator()(std::set<benchmark_result>& benchmark_results) const;
    };
  }  // namespace detail

  template<typename T>
  void run_benchmark(std::set<benchmark_result>& benchmark_results)
  {
    detail::benchmark_impl<T>{}(benchmark_results);
  }
}  // namespace quxflux

#define IMPL_BENCHMARK(T)                                                                                              \
  template<>                                                                                                           \
  void detail::benchmark_impl<T>::operator()(std::set<benchmark_result>& benchmark_results) const                    \
  {                                                                                                                    \
    benchmark_all_with_size_and_type<1, T>(benchmark_results);                                                       \
    benchmark_all_with_size_and_type<2, T>(benchmark_results);                                                       \
    benchmark_all_with_size_and_type<4, T>(benchmark_results);                                                       \
    benchmark_all_with_size_and_type<8, T>(benchmark_results);                                                       \
    benchmark_all_with_size_and_type<16, T>(benchmark_results);                                                      \
    benchmark_all_with_size_and_type<32, T>(benchmark_results);                                                      \
    benchmark_all_with_size_and_type<64, T>(benchmark_results);                                                      \
    benchmark_all_with_size_and_type<128, T>(benchmark_results);                                                     \
                                                                                                                       \
    std::clog << '\n';                                                                                                 \
  }                                                                                                                    \
  static_assert(true, "force trailing semicolon")
