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

#pragma once

#include <sorting_network_cpp/sorting_network.h>

#include <gtest/gtest.h>

#include <metal.hpp>

#include <array>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <string>
#include <utility>

namespace quxflux::sorting_net
{
  static inline constexpr std::size_t max_networks_to_test =
#if defined(_MSC_VER)
    17  // if chosen too large, msbuild will fail with "fatal error C1202: recursive type or function dependency context
        // too complex"
#else
    35
#endif
    ;

  struct custom_type
  {
    std::array<std::int32_t, 2> values;

    constexpr bool operator<(const custom_type& rhs) const
    {
      return std::tie(values[1], values[0]) < std::tie(rhs.values[1], rhs.values[0]);
    }

    constexpr bool operator>(const custom_type& rhs) const
    {
      return std::tie(values[1], values[0]) > std::tie(rhs.values[1], rhs.values[0]);
    }

    constexpr bool operator==(const custom_type& rhs) const
    {
      return std::tie(values[1], values[0]) == std::tie(rhs.values[1], rhs.values[0]);
    }
  };

  template<type Network>
  constexpr std::string_view to_string()
  {
    if constexpr (Network == type::batcher_odd_even_merge_sort)
      return "batcher_odd_even_merge_sort";
    if constexpr (Network == type::bitonic_merge_sort)
      return "bitonic_merge_sort";
    if constexpr (Network == type::bose_nelson_sort)
      return "bose_nelson_sort";
    if constexpr (Network == type::bubble_sort)
      return "bubble_sort";
    if constexpr (Network == type::insertion_sort)
      return "insertion_sort";
    if constexpr (Network == type::size_optimized_sort)
      return "size_optimized_sort";
  }

  template<typename Type>
  constexpr std::string_view to_string()
  {
    if constexpr (std::is_same_v<Type, std::uint8_t>)
      return "uint8_t";
    if constexpr (std::is_same_v<Type, float>)
      return "float";
    if constexpr (std::is_same_v<Type, custom_type>)
      return "custom_type";
  }

  template<typename NetworkT, typename ArraySize, typename ValueType>
  struct test_spec
  {
    static inline constexpr type network_type = NetworkT::value;
    static inline constexpr std::size_t array_size = ArraySize::value;
    using value_type = ValueType;

    using sorting_network_type = sorting_network<array_size, network_type>;
  };

  struct test_type_name_generator
  {
    template<typename test_spec>
    static std::string GetName(int)
    {
      std::stringstream ss;

      ss << to_string<test_spec::network_type>();
      ss << "_" << std::setfill('0') << std::setw(2) << test_spec::array_size;
      ss << "_" << to_string<typename test_spec::value_type>();

      return ss.str();
    }
  };

  template<typename TypeList, template<typename> typename Predicate>
  using filter_list = metal::copy_if<TypeList, metal::trait<Predicate>>;

  template<typename TestSpec>
  using is_valid_test_spec = available<TestSpec::array_size, TestSpec::network_type>;

  template<type Network>
  using network_t = std::integral_constant<type, Network>;

  using all_network_types =
    metal::list<network_t<type::batcher_odd_even_merge_sort>, network_t<type::bitonic_merge_sort>,
                network_t<type::bose_nelson_sort>, network_t<type::bubble_sort>, network_t<type::insertion_sort>,
                network_t<type::size_optimized_sort>>;

  using array_sizes = metal::iota<metal::number<1>, metal::number<max_networks_to_test>>;
  using value_types = metal::list<std::uint8_t, float, custom_type>;

  using sizes_and_value_types =
    filter_list<metal::transform<metal::partial<metal::lambda<metal::apply>, metal::lambda<test_spec>>,
                                 metal::cartesian<all_network_types, array_sizes, value_types>>,
                is_valid_test_spec>;

  template<typename... TestSpecs>
  auto generate_google_test_specs(const metal::list<TestSpecs...>) -> ::testing::Types<TestSpecs...>
  {
    return {};
  }

  using test_specs = decltype(generate_google_test_specs(sizes_and_value_types{}));

}  // namespace quxflux::sorting_net
