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

#include <chrono>
#include <cstddef>
#include <map>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>

struct Vec2i : std::array<uint16_t, 2>
{
  // A slightly more complex comparison operation:
  // compare the morton code of two points (by generating the
  // morton codes on the fly)

  static constexpr uint16_t ExpandBits(uint16_t x) noexcept
  {
    x &= 0x000003ff;                   // x = ---- ---- ---- ---- ---- --98 7654 3210
    x = (x ^ (x << 16)) & 0x030000ff;  // x = ---- --98 ---- ---- ---- ---- 7654 3210
    x = (x ^ (x << 8)) & 0x0300f00f;   // x = ---- --98 ---- ---- 7654 ---- ---- 3210
    x = (x ^ (x << 4)) & 0x030c30c3;   // x = ---- --98 ---- 76-- --54 ---- 32-- --10
    x = (x ^ (x << 2)) & 0x09249249;   // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0

    return x;
  }

  static constexpr uint32_t GenerateMortonCode2d(const Vec2i& p) noexcept
  {
    return (ExpandBits(p[0]) << 1 | ExpandBits(p[1]));
  }

  constexpr bool operator()(const Vec2i& lhs, const Vec2i& rhs) const noexcept
  {
    return GenerateMortonCode2d(lhs) < GenerateMortonCode2d(rhs);
  }
};

using algorithm_name = std::string_view;

using duration_t = std::chrono::duration<double, std::milli>;

template<quxflux::sorting_net::type NWT>
using network = std::integral_constant<quxflux::sorting_net::type, NWT>;

struct benchmark_result
{
  std::string_view data_type;
  std::size_t n = 0;
  algorithm_name algorithm;

  std::optional<duration_t> avg_exec_time;

  constexpr bool operator<(const benchmark_result& rhs) const
  {
    return std::tie(data_type, n, algorithm) < std::tie(rhs.data_type, rhs.n, rhs.algorithm);
  }
};

struct benchmark_scenario
{
  std::string_view data_type;
  std::size_t n = 0;

  constexpr bool operator==(const benchmark_scenario& rhs) const { return data_type == rhs.data_type && n == rhs.n; }

  constexpr bool operator<(const benchmark_scenario& rhs) const
  {
    return std::tie(data_type, n) < std::tie(rhs.data_type, rhs.n);
  }
};

using benchmark_table = std::map<benchmark_scenario, std::map<algorithm_name, std::optional<duration_t>>>;

template<typename T, std::size_t N>
std::vector<std::array<T, N>> generate_benchmark_data()
{
  std::vector<std::array<T, N>> unsorted_data(1'000'000);
  std::default_random_engine rd(42);

  for (auto& array : unsorted_data)
  {
    std::generate(array.begin(), array.end(), [&]() -> T {
      if constexpr (std::is_same_v<Vec2i, T>)
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

namespace detail
{
  template<typename F, typename... Types>
  void static_for_each_impl(const F f, const std::tuple<Types...>)
  {
    (f(Types{}), ...);
  }

  template<typename F, std::size_t... Indices>
  void static_for_impl(const F f, const std::index_sequence<Indices...>)
  {
    (f(std::integral_constant<std::size_t, Indices>{}), ...);
  }
}  // namespace detail

template<typename TypeList, typename F>
void static_for_each(const F f)
{
  detail::static_for_each_impl(f, TypeList{});
}

template<std::size_t N, typename F>
void static_for(const F f)
{
  detail::static_for_impl(f, std::make_index_sequence<N>{});
}

template<typename Duration, typename F, typename... Args>
Duration measure_execution_time(const F& f, Args&&... args)
{
  using clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock,
                                   std::chrono::steady_clock>;

  const auto start = clock::now();
  f(std::forward<Args>(args)...);
  const auto end = clock::now();

  return std::chrono::duration_cast<Duration>(end - start);
}

static inline const std::string_view compiler_id = []() -> std::string_view {
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

  static std::string s = os.str();

  return s;
}();

template<typename T>
constexpr std::string_view to_string()
{
  if constexpr (std::is_same_v<int16_t, T>)
    return "int16_t";
  if constexpr (std::is_same_v<int32_t, T>)
    return "int32_t";
  if constexpr (std::is_same_v<uint32_t, T>)
    return "uint32_t";
  if constexpr (std::is_same_v<int64_t, T>)
    return "int64_t";
  if constexpr (std::is_same_v<float, T>)
    return "float";
  if constexpr (std::is_same_v<double, T>)
    return "double";
  if constexpr (std::is_same_v<Vec2i, T>)
    return "vec2i Z-order";
}

template<quxflux::sorting_net::type NWT>
constexpr std::string_view to_string()
{
  if constexpr (NWT == quxflux::sorting_net::type::bubble_sort)
    return "SN::bubble_sort";
  if constexpr (NWT == quxflux::sorting_net::type::insertion_sort)
    return "SN::insertion_sort";
  if constexpr (NWT == quxflux::sorting_net::type::batcher_odd_even_merge_sort)
    return "SN::batcher_odd_even_merge_sort";
  if constexpr (NWT == quxflux::sorting_net::type::bitonic_merge_sort)
    return "SN::bitonic_merge_sort";
  if constexpr (NWT == quxflux::sorting_net::type::size_optimized_sort)
    return "SN::size_optimized_sort";
  if constexpr (NWT == quxflux::sorting_net::type::bose_nelson_sort)
    return "SN::bose_nelson_sort";
}

inline std::ostream& operator<<(std::ostream& os, const benchmark_table& table)
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

#if defined(_MSC_VER)
inline constexpr bool is_msvc = true;
#else
inline constexpr bool is_msvc = false;
#endif
