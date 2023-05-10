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

#include <sorting_network_cpp/networks/common.h>

#include <gmock/gmock.h>

#include <metal.hpp>

#include <algorithm>
#include <array>
#include <iomanip>
#include <random>
#include <sstream>
#include <string_view>
#include <string>
#include <utility>

namespace quxflux::sorting_net
{
  static inline constexpr std::size_t max_array_size_to_test = 35;

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

  template<size_t MaxSizes>
  using array_sizes = metal::iota<metal::number<1>, metal::number<MaxSizes>>;
  using value_types = metal::list<std::uint8_t, float, custom_type>;

  template<type Network, size_t MaxSize>
  using sizes_and_value_types =
    filter_list<metal::transform<metal::partial<metal::lambda<metal::apply>, metal::lambda<test_spec>>,
                                 metal::cartesian<metal::list<network_t<Network>>, array_sizes<MaxSize>, value_types>>,
                is_valid_test_spec>;

  template<typename... TestSpecs>
  auto generate_google_test_specs(const metal::list<TestSpecs...>) -> ::testing::Types<TestSpecs...>
  {
    return {};
  }

  template<type Network, size_t MaxSize = max_array_size_to_test>
  using test_specs_for_network = decltype(generate_google_test_specs(sizes_and_value_types<Network, MaxSize>{}));

  template<typename It, typename RandomDevice>
  void fill_random(const It begin, const It end, RandomDevice& rd)
  {
    using value_t = typename std::iterator_traits<It>::value_type;

    std::generate(begin, end, [&]() {
      value_t v;
      if constexpr (std::is_floating_point_v<value_t>)
      {
        v = std::uniform_real_distribution<value_t>{}(rd);

      } else if constexpr (std::is_same_v<custom_type, value_t>)
      {
        fill_random(v.values.begin(), v.values.end(), rd);
      } else
      {
        v = static_cast<value_t>(std::uniform_int_distribution<std::int64_t>{}(rd));
      }
      return v;
    });
  }

  template<typename TestSpec>
  struct sorting_network_test : ::testing::Test
  {
    using array_t = std::array<typename TestSpec::value_type, TestSpec::array_size>;

    sorting_network_test() {}

    const array_t input_array = [] {
      array_t r;

      std::default_random_engine rd;

      fill_random(r.begin(), r.end(), rd);
      return r;
    }();

    array_t array = input_array;
  };

  TYPED_TEST_SUITE_P(sorting_network_test);

  TYPED_TEST_P(sorting_network_test, array_is_ordered_when_input_is_random_ordered)
  {
    typename TypeParam::sorting_network_type{}(this->array.begin());

    EXPECT_TRUE(std::is_sorted(this->array.begin(), this->array.end()));
    EXPECT_TRUE(std::is_permutation(this->array.begin(), this->array.end(), this->input_array.begin()));
  }

  TYPED_TEST_P(sorting_network_test, array_is_ordered_when_input_is_reverse_ordered)
  {
    std::sort(this->array.begin(), this->array.end(), std::greater{});

    typename TypeParam::sorting_network_type{}(this->array.begin());

    EXPECT_TRUE(std::is_sorted(this->array.begin(), this->array.end()));
    EXPECT_TRUE(std::is_permutation(this->array.begin(), this->array.end(), this->input_array.begin()));
  }

  TYPED_TEST_P(sorting_network_test, array_is_ordered_when_input_is_ordered)
  {
    std::sort(this->array.begin(), this->array.end());

    typename TypeParam::sorting_network_type{}(this->array.begin());

    EXPECT_TRUE(std::is_sorted(this->array.begin(), this->array.end()));
    EXPECT_TRUE(std::is_permutation(this->array.begin(), this->array.end(), this->input_array.begin()));
  }

  TYPED_TEST_P(sorting_network_test, array_is_ordered_when_using_custom_compare_and_swap_operator)
  {
    typename TypeParam::sorting_network_type{}(this->array.begin(), [](auto& a, auto& b) {
      const auto b_cpy = b;
      b = std::max(a, b);
      a = std::min(a, b_cpy);
    });

    EXPECT_TRUE(std::is_sorted(this->array.begin(), this->array.end()));
    EXPECT_TRUE(std::is_permutation(this->array.begin(), this->array.end(), this->input_array.begin()));
  }

  TYPED_TEST_P(sorting_network_test, array_is_ordered_when_using_custom_predicate)
  {
    using custom_predicate = std::greater<typename TypeParam::value_type>;

    typename TypeParam::sorting_network_type{}(this->array.begin(),
                                               compare_and_swap<typename TypeParam::value_type, custom_predicate>{});

    EXPECT_TRUE(std::is_sorted(this->array.begin(), this->array.end(), custom_predicate{}));
    EXPECT_TRUE(std::is_permutation(this->array.begin(), this->array.end(), this->input_array.begin()));
  }

  REGISTER_TYPED_TEST_SUITE_P(sorting_network_test,                                          //
                              array_is_ordered_when_input_is_random_ordered,                 //
                              array_is_ordered_when_input_is_reverse_ordered,                //
                              array_is_ordered_when_input_is_ordered,                        //
                              array_is_ordered_when_using_custom_compare_and_swap_operator,  //
                              array_is_ordered_when_using_custom_predicate);
}  // namespace quxflux::sorting_net
