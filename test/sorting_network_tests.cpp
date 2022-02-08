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

#include <sorting_network_cpp/sorting_network.h>

#include "test_case_gen.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <random>

namespace quxflux::sorting_net
{
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

  TYPED_TEST_SUITE(sorting_network_test, test_specs, test_type_name_generator);

  TYPED_TEST(sorting_network_test, array_is_ordered_when_input_is_random_ordered)
  {
    typename TypeParam::sorting_network_type{}(this->array.begin());

    EXPECT_TRUE(std::is_sorted(this->array.begin(), this->array.end()));
    EXPECT_TRUE(std::is_permutation(this->array.begin(), this->array.end(), this->input_array.begin()));
  }

  TYPED_TEST(sorting_network_test, array_is_ordered_when_input_is_reverse_ordered)
  {
    std::sort(this->array.begin(), this->array.end(), std::greater{});

    typename TypeParam::sorting_network_type{}(this->array.begin());

    EXPECT_TRUE(std::is_sorted(this->array.begin(), this->array.end()));
    EXPECT_TRUE(std::is_permutation(this->array.begin(), this->array.end(), this->input_array.begin()));
  }

  TYPED_TEST(sorting_network_test, array_is_ordered_when_input_is_ordered)
  {
    std::sort(this->array.begin(), this->array.end());

    typename TypeParam::sorting_network_type{}(this->array.begin());

    EXPECT_TRUE(std::is_sorted(this->array.begin(), this->array.end()));
    EXPECT_TRUE(std::is_permutation(this->array.begin(), this->array.end(), this->input_array.begin()));
  }

  TYPED_TEST(sorting_network_test, array_is_ordered_when_using_custom_compare_and_swap_operator)
  {
    typename TypeParam::sorting_network_type{}(this->array.begin(), [](auto& a, auto& b) {
      const auto b_cpy = b;
      b = std::max(a, b);
      a = std::min(a, b_cpy);
    });

    EXPECT_TRUE(std::is_sorted(this->array.begin(), this->array.end()));
    EXPECT_TRUE(std::is_permutation(this->array.begin(), this->array.end(), this->input_array.begin()));
  }

  TYPED_TEST(sorting_network_test, array_is_ordered_when_using_custom_predicate)
  {
    using custom_predicate = std::greater<typename TypeParam::value_type>;

    typename TypeParam::sorting_network_type{}(this->array.begin(),
                                               compare_and_swap<typename TypeParam::value_type, custom_predicate>{});

    EXPECT_TRUE(std::is_sorted(this->array.begin(), this->array.end(), custom_predicate{}));
    EXPECT_TRUE(std::is_permutation(this->array.begin(), this->array.end(), this->input_array.begin()));
  }
}  // namespace quxflux::sorting_net
