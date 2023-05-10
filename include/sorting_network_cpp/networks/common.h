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

#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

namespace quxflux::sorting_net
{
  constexpr std::size_t operator"" _z(unsigned long long n)
  {
    return std::size_t{n};
  }

  enum class type
  {
    insertion_sort,
    bubble_sort,
    bose_nelson_sort,
    batcher_odd_even_merge_sort,
    bitonic_merge_sort,
    size_optimized_sort
  };

  template<typename T, typename Predicate>
  struct compare_and_swap
  {
    constexpr void operator()(T& a, T& b) const noexcept
    {
      const T t = Predicate{}(a, b) ? a : b;
      b = Predicate{}(a, b) ? b : a;
      a = t;
    }
  };

  namespace detail
  {
    template<typename It, typename CAS>
    struct iterator_compare_and_swap;

    template<typename It>
    static inline constexpr bool is_random_access_iterator_v =
      std::is_same_v<typename std::iterator_traits<It>::iterator_category, std::random_access_iterator_tag>;
  }  // namespace detail

  template<std::size_t A, std::size_t B>
  struct cas_node
  {};

  template<typename... Pairs>
  struct layer
  {};

  template<typename... Layers>
  struct net
  {};

  template<typename It, typename CAS, std::size_t A, std::size_t B>
  constexpr void apply(const It it, const CAS& cas_f, const cas_node<A, B>) noexcept
  {
    detail::iterator_compare_and_swap<It, CAS>()(it + A, it + B, cas_f);
  }

  template<typename It, typename CAS, typename... LayerOrCas>
  constexpr void apply(const It it, const CAS& cas, const layer<LayerOrCas...>) noexcept
  {
    (apply(it, cas, LayerOrCas{}), ...);
  }

  template<typename It, typename CAS, typename... Layers>
  constexpr void apply(const It it, const CAS& cas, const net<Layers...>) noexcept
  {
    (apply<It, CAS>(it, cas, Layers{}), ...);
  }

  namespace detail
  {
    template<typename It, typename CAS>
    struct iterator_compare_and_swap
    {
      constexpr void operator()(const It a, const It b, const CAS& compare_and_swap) const noexcept
      {
        compare_and_swap(*a, *b);
      }
    };

    constexpr bool is_power_of_two(const std::size_t n)
    {
      return (n > 0) && !(n & (n - 1));
    }

    constexpr std::size_t next_smallest_power_of_two(const std::size_t n)
    {
      std::size_t result = 1;

      while (result < n)
        result <<= 1;

      return result >>= 1;
    }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    constexpr T int_div_ceil(const T x, const T y)
    {
      return T{1} + ((x - T{1}) / y);
    }

    template<std::size_t N, type NWT, typename = void>
    struct sorting_network;
  }  // namespace detail

  template<std::size_t N, type Network, typename = void>
  struct available : std::false_type
  {};

  template<std::size_t N, type Network>
  struct available<N, Network, std::void_t<decltype(detail::sorting_network<N, Network>{})>> : std::true_type
  {};

  template<std::size_t N, type Network>
  constexpr bool available_v = available<N, Network>::value;

  template<std::size_t N, type NetworkType = type::bose_nelson_sort>
  struct sorting_network
  {
    static_assert(N > 0);

    template<typename It,
             typename CompareAndSwap = compare_and_swap<typename std::iterator_traits<It>::value_type,
                                                        std::less<typename std::iterator_traits<It>::value_type>>,
             typename = std::enable_if_t<detail::is_random_access_iterator_v<It>>>
    constexpr void operator()(const It begin, const CompareAndSwap& cas = {}) const noexcept
    {
      if constexpr (N > 1)
        apply(begin, cas, typename detail::sorting_network<N, NetworkType>::type{});
    }
  };
}  // namespace quxflux::sorting_net
