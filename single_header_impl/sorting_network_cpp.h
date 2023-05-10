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

namespace quxflux::sorting_net::detail
{
  // Generates a sorting network based the construction scheme
  // by Ken Batcher
  template<std::size_t N>
  struct sorting_network<N, type::batcher_odd_even_merge_sort, std::enable_if_t<is_power_of_two(N)>>
  {
  private:
    template<std::size_t I, std::size_t J, std::size_t R, typename = void>
    struct gen_merge
    {
      using type = sorting_net::layer<sorting_net::cas_node<I, (I + R)>>;
    };

    template<std::size_t I, std::size_t J, std::size_t R>
    struct gen_merge<I, J, R, std::enable_if_t<(R * 2_z <= J - I)>>
    {
      static constexpr std::size_t step = R * 2_z;

      template<std::size_t... Indices>
      static auto gen_cas(std::index_sequence<Indices...>)
        -> sorting_net::layer<sorting_net::cas_node<I + R + Indices * step, I + R + Indices * step + R>...>
      {
        return {};
      }

      using type =
        sorting_net::layer<typename gen_merge<I, J, step>::type, typename gen_merge<I + R, J, step>::type,
                           decltype(gen_cas(std::make_index_sequence<int_div_ceil((J - R) - (I + R), step)>()))>;
    };

    template<std::size_t Lo, std::size_t Hi, typename = void>
    struct gen_sort
    {
      using type = sorting_net::layer<>;
    };

    template<std::size_t Lo, std::size_t Hi>
    struct gen_sort<Lo, Hi, std::enable_if_t<(Hi - Lo >= 1_z)>>
    {
      static constexpr std::size_t Mid = Lo + (Hi - Lo) / 2_z;

      using type = sorting_net::layer<           //
        typename gen_sort<Lo, Mid>::type,        //
        typename gen_sort<Mid + 1_z, Hi>::type,  //
        typename gen_merge<Lo, Hi, 1_z>::type    //
        >;
    };

  public:
    using type = sorting_net::net<typename gen_sort<0_z, N - 1_z>::type>;
  };
}  // namespace quxflux::sorting_net

namespace quxflux::sorting_net::detail
{
  // Generates a bitonic sorting network
  template<std::size_t N>
  struct sorting_network<N, type::bitonic_merge_sort, void>
  {
  private:
    template<std::size_t Lo, std::size_t Hi, bool Inv, typename = void>
    struct gen_merge
    {
      using type = sorting_net::layer<>;
    };

    template<std::size_t Lo, std::size_t Hi, bool Inv>
    struct gen_merge<Lo, Hi, Inv, std::enable_if_t<(Hi - Lo >= 1_z)>>
    {
      static inline constexpr std::size_t n = (Hi - Lo + 1_z);

      // calculate m as the next smallest power of 2 for n
      static inline constexpr std::size_t m = static_cast<std::size_t>(next_smallest_power_of_two(n));

      template<std::size_t... Indices>
      static auto make_cas(const std::index_sequence<Indices...>)
        -> sorting_net::layer<std::conditional_t<Inv, sorting_net::cas_node<Lo + Indices + m, Lo + Indices>,
                                                 sorting_net::cas_node<Lo + Indices, Lo + Indices + m>>...>
      {
        return {};
      };

      using type = sorting_net::layer<                            //
        decltype(make_cas(std::make_index_sequence<(n - m)>())),  //
        typename gen_merge<Lo, Lo + m - 1_z, Inv>::type,          //
        typename gen_merge<Lo + m, Hi, Inv>::type                 //
        >;
    };

    template<std::size_t Lo, std::size_t Hi, bool Inv, typename = void>
    struct gen_sort
    {
      using type = sorting_net::layer<>;
    };

    template<std::size_t Lo, std::size_t Hi, bool Inv>
    struct gen_sort<Lo, Hi, Inv, std::enable_if_t<(Hi - Lo >= 1_z)>>
    {
      static inline constexpr std::size_t Mid = Lo + (Hi - Lo + 1_z) / 2_z;

      using type = sorting_net::layer<                 //
        typename gen_sort<Lo, Mid - 1_z, !Inv>::type,  //
        typename gen_sort<Mid, Hi, Inv>::type,         //
        typename gen_merge<Lo, Hi, Inv>::type          //
        >;
    };

  public:
    using type = sorting_net::net<typename gen_sort<0_z, N - 1_z, false>::type>;
  };
}  // namespace quxflux::sorting_net

namespace quxflux::sorting_net::detail
{
  // Generates a sorting network based on the algorithm proposed
  // by Bose and Nelson in "A Sorting Problem" (1962),
  // adapted from https://github.com/Vectorized/Static-Sort
  template<std::size_t N>
  struct sorting_network<N, type::bose_nelson_sort>
  {
  private:
    template<std::size_t I, std::size_t J, std::size_t X, std::size_t Y, typename = void>
    struct gen_p
    {
      static inline constexpr std::size_t L = X / 2_z;
      static inline constexpr std::size_t M = (X & 1_z ? Y : Y + 1_z) / 2_z;

      using type = sorting_net::layer<                     //
        typename gen_p<I, J, L, M>::type,                  //
        typename gen_p<I + L, J + M, X - L, Y - M>::type,  //
        typename gen_p<I + L, J, X - L, M>::type           //
        >;
    };

    template<std::size_t I, std::size_t J, std::size_t X, std::size_t Y>
    struct gen_p<I, J, X, Y, std::enable_if_t<(X == 1_z && Y == 1_z)>>
    {
      using type = sorting_net::layer<sorting_net::cas_node<I - 1_z, J - 1_z>>;
    };

    template<std::size_t I, std::size_t J, std::size_t X, std::size_t Y>
    struct gen_p<I, J, X, Y, std::enable_if_t<(X == 1_z && Y == 2_z)>>
    {
      using type = sorting_net::layer<           //
        sorting_net::cas_node<I - 1_z, J>,       //
        sorting_net::cas_node<I - 1_z, J - 1_z>  //
        >;
    };

    template<std::size_t I, std::size_t J, std::size_t X, std::size_t Y>
    struct gen_p<I, J, X, Y, std::enable_if_t<(X == 2_z && Y == 1_z)>>
    {
      using type = sorting_net::layer<            //
        sorting_net::cas_node<I - 1_z, J - 1_z>,  //
        sorting_net::cas_node<I, J - 1_z>         //
        >;
    };

    template<std::size_t Lo, std::size_t N_, typename = void>
    struct gen_p_star
    {
      using type = sorting_net::layer<>;
    };

    template<std::size_t Lo, std::size_t N_>
    struct gen_p_star<Lo, N_, std::enable_if_t<(N_ > 1_z)>>
    {
      static inline constexpr std::size_t M = N_ / 2_z;

      template<bool Enable, typename = void>
      struct t0
      {
        using type = sorting_net::layer<>;
      };

      template<bool Enable>
      struct t0<Enable, std::enable_if_t<Enable>>
      {
        using type = typename gen_p_star<Lo, M>::type;
      };

      template<bool Enable, typename = void>
      struct t1
      {
        using type = sorting_net::layer<>;
      };

      template<bool Enable>
      struct t1<Enable, std::enable_if_t<Enable>>
      {
        using type = typename gen_p_star<Lo + M, N_ - M>::type;
      };

      using type = sorting_net::layer<               //
        typename t0<(M > 1_z)>::type,                //
        typename t1<(N_ - M > 1_z)>::type,           //
        typename gen_p<Lo, Lo + M, M, N_ - M>::type  //
        >;
    };

  public:
    using type = sorting_net::net<typename gen_p_star<1, N>::type>;
  };
}  // namespace quxflux::sorting_net

namespace quxflux::sorting_net::detail
{
    // Generates a bubble sort sorting network of following form
    // (0,1) (1,2) (2,3) ... (N-2,N-1)
    // ...
    // (0,1) (1,2) (2,3)
    // (0,1) (1,2)
    // (0,1)
    template<std::size_t N>
    struct sorting_network<N, type::bubble_sort, void>
    {
    private:
        template<std::size_t... PairIndices>
        static auto gen_layer_pairs(const std::index_sequence<PairIndices...>)
            -> sorting_net::layer<sorting_net::cas_node<PairIndices, PairIndices + 1_z>...>
        {
            return {};
        }

        template<std::size_t... LayerIndices>
        static auto gen_layers(const std::index_sequence<LayerIndices...>) -> sorting_net::net<
            decltype(gen_layer_pairs(std::make_index_sequence<sizeof...(LayerIndices) - LayerIndices - 1_z>()))...>
        {
            return {};
        };

    public:
        using type = decltype(gen_layers(std::make_index_sequence<N>()));
    };
}

namespace quxflux::sorting_net::detail
{
  // Generates an insertion sort sorting network of following form
  // (0,1)
  // (1,2) (0,1)
  // (2,3) (1,2) (0,1)
  // ...
  // (N-2,N-1) ... (2,3) (1,2) (0,1)
  template<std::size_t N>
  struct sorting_network<N, type::insertion_sort, void>
  {
  private:
    template<std::size_t... PairIndices>
    static auto gen_layer_pairs(const std::index_sequence<PairIndices...>) -> sorting_net::layer<
      sorting_net::cas_node<sizeof...(PairIndices) - PairIndices - 1_z, sizeof...(PairIndices) - PairIndices>...>
    {
      return {};
    }

    template<std::size_t... LayerIndices>
    static auto gen_layers(const std::index_sequence<LayerIndices...>)
      -> sorting_net::net<decltype(gen_layer_pairs(std::make_index_sequence<LayerIndices>()))...>
    {
      return {};
    };

  public:
    using type = decltype(gen_layers(std::make_index_sequence<N>()));
  };
}  // namespace quxflux::sorting_net::detail

namespace quxflux::sorting_net::detail
{
  namespace sorter_hunter
  {
    template<std::size_t A, std::size_t B>
    using P = sorting_net::cas_node<A, B>;

    template<typename... Pairs>
    using L = sorting_net::layer<Pairs...>;

    template<typename... Layers>
    using N = sorting_net::net<Layers...>;

    // clang-format off
    using Networks = std::tuple<
                /* 1 */  N<L<>>,
                /* 2 */  N<L<P<0,1>>>,
                /* 3 */  N<L<P<0,2>>,
                          L<P<0,1>>,
                          L<P<1,2>>>,
                /* 4 */  N<L<P<0,2>,P<1,3>>,
                          L<P<0,1>,P<2,3>>,
                          L<P<1,2>>>,
                /* 5 */  N<L<P<0,3>,P<1,4>>,
                          L<P<0,2>,P<1,3>>,
                          L<P<0,1>,P<2,4>>,
                          L<P<1,2>,P<3,4>>,
                          L<P<2,3>>>,
                /* 6 */  N<L<P<0,5>,P<1,3>,P<2,4>>,
                          L<P<1,2>,P<3,4>>,
                          L<P<0,3>,P<2,5>>,
                          L<P<0,1>,P<2,3>,P<4,5>>,
                          L<P<1,2>,P<3,4>>>,
                /* 7 */  N<L<P<0,6>,P<2,3>,P<4,5>>,
                          L<P<0,2>,P<1,4>,P<3,6>>,
                          L<P<0,1>,P<2,5>,P<3,4>>,
                          L<P<1,2>,P<4,6>>,
                          L<P<2,3>,P<4,5>>,
                          L<P<1,2>,P<3,4>,P<5,6>>>,
                /* 8 */  N<L<P<0,2>,P<1,3>,P<4,6>,P<5,7>>,
                          L<P<0,4>,P<1,5>,P<2,6>,P<3,7>>,
                          L<P<0,1>,P<2,3>,P<4,5>,P<6,7>>,
                          L<P<2,4>,P<3,5>>,
                          L<P<1,4>,P<3,6>>,
                          L<P<1,2>,P<3,4>,P<5,6>>>,
                /* 9 */  N<L<P<0,3>,P<1,7>,P<2,5>,P<4,8>>,
                          L<P<0,7>,P<2,4>,P<3,8>,P<5,6>>,
                          L<P<0,2>,P<1,3>,P<4,5>,P<7,8>>,
                          L<P<1,4>,P<3,6>,P<5,7>>,
                          L<P<0,1>,P<2,4>,P<3,5>,P<6,8>>,
                          L<P<2,3>,P<4,5>,P<6,7>>,
                          L<P<1,2>,P<3,4>,P<5,6>>>,
                /* 10 */ N<L<P<0,8>,P<1,9>,P<2,7>,P<3,5>,P<4,6>>,
                          L<P<0,2>,P<1,4>,P<5,8>,P<7,9>>,
                          L<P<0,3>,P<2,4>,P<5,7>,P<6,9>>,
                          L<P<0,1>,P<3,6>,P<8,9>>,
                          L<P<1,5>,P<2,3>,P<4,8>,P<6,7>>,
                          L<P<1,2>,P<3,5>,P<4,6>,P<7,8>>,
                          L<P<2,3>,P<4,5>,P<6,7>>,
                          L<P<3,4>,P<5,6>>>,
                /* 11 */ N<L<P<0,9>,P<1,6>,P<2,4>,P<3,7>,P<5,8>>,
                          L<P<0,1>,P<3,5>,P<4,10>,P<6,9>,P<7,8>>,
                          L<P<1,3>,P<2,5>,P<4,7>,P<8,10>>,
                          L<P<0,4>,P<1,2>,P<3,7>,P<5,9>,P<6,8>>,
                          L<P<0,1>,P<2,6>,P<4,5>,P<7,8>,P<9,10>>,
                          L<P<2,4>,P<3,6>,P<5,7>,P<8,9>>,
                          L<P<1,2>,P<3,4>,P<5,6>,P<7,8>>,
                          L<P<2,3>,P<4,5>,P<6,7>>>,
                /* 12 */ N<L<P<0,8>,P<1,7>,P<2,6>,P<3,11>,P<4,10>,P<5,9>>,
                          L<P<0,1>,P<2,5>,P<3,4>,P<6,9>,P<7,8>,P<10,11>>,
                          L<P<0,2>,P<1,6>,P<5,10>,P<9,11>>,
                          L<P<0,3>,P<1,2>,P<4,6>,P<5,7>,P<8,11>,P<9,10>>,
                          L<P<1,4>,P<3,5>,P<6,8>,P<7,10>>,
                          L<P<1,3>,P<2,5>,P<6,9>,P<8,10>>,
                          L<P<2,3>,P<4,5>,P<6,7>,P<8,9>>,
                          L<P<4,6>,P<5,7>>,
                          L<P<3,4>,P<5,6>,P<7,8>>>,
                /* 13 */ N<L<P<0,12>,P<1,10>,P<2,9>,P<3,7>,P<5,11>,P<6,8>>,
                          L<P<1,6>,P<2,3>,P<4,11>,P<7,9>,P<8,10>>,
                          L<P<0,4>,P<1,2>,P<3,6>,P<7,8>,P<9,10>,P<11,12>>,
                          L<P<4,6>,P<5,9>,P<8,11>,P<10,12>>,
                          L<P<0,5>,P<3,8>,P<4,7>,P<6,11>,P<9,10>>,
                          L<P<0,1>,P<2,5>,P<6,9>,P<7,8>,P<10,11>>,
                          L<P<1,3>,P<2,4>,P<5,6>,P<9,10>>,
                          L<P<1,2>,P<3,4>,P<5,7>,P<6,8>>,
                          L<P<2,3>,P<4,5>,P<6,7>,P<8,9>>,
                          L<P<3,4>,P<5,6>>>,
                /* 14 */ N<L<P<0,6>,P<1,11>,P<2,12>,P<3,10>,P<4,5>,P<7,13>,P<8,9>>,
                          L<P<1,2>,P<3,7>,P<4,8>,P<5,9>,P<6,10>,P<11,12>>,
                          L<P<0,4>,P<1,3>,P<5,6>,P<7,8>,P<9,13>,P<10,12>>,
                          L<P<0,1>,P<2,9>,P<3,7>,P<4,11>,P<6,10>,P<12,13>>,
                          L<P<2,5>,P<4,7>,P<6,9>,P<8,11>>,
                          L<P<1,2>,P<3,4>,P<6,7>,P<9,10>,P<11,12>>,
                          L<P<1,3>,P<2,4>,P<5,6>,P<7,8>,P<9,11>,P<10,12>>,
                          L<P<2,3>,P<4,7>,P<6,9>,P<10,11>>,
                          L<P<4,5>,P<6,7>,P<8,9>>,
                          L<P<3,4>,P<5,6>,P<7,8>,P<9,10>>>,
                /* 15 */ N<L<P<1,2>,P<3,10>,P<4,14>,P<5,8>,P<6,13>,P<7,12>,P<9,11>>,
                          L<P<0,14>,P<1,5>,P<2,8>,P<3,7>,P<6,9>,P<10,12>,P<11,13>>,
                          L<P<0,7>,P<1,6>,P<2,9>,P<4,10>,P<5,11>,P<8,13>,P<12,14>>,
                          L<P<0,6>,P<2,4>,P<3,5>,P<7,11>,P<8,10>,P<9,12>,P<13,14>>,
                          L<P<0,3>,P<1,2>,P<4,7>,P<5,9>,P<6,8>,P<10,11>,P<12,13>>,
                          L<P<0,1>,P<2,3>,P<4,6>,P<7,9>,P<10,12>,P<11,13>>,
                          L<P<1,2>,P<3,5>,P<8,10>,P<11,12>>,
                          L<P<3,4>,P<5,6>,P<7,8>,P<9,10>>,
                          L<P<2,3>,P<4,5>,P<6,7>,P<8,9>,P<10,11>>,
                          L<P<5,6>,P<7,8>>>,
                /* 16 */ N<L<P<0,13>,P<1,12>,P<2,15>,P<3,14>,P<4,8>,P<5,6>,P<7,11>,P<9,10>>,
                          L<P<0,5>,P<1,7>,P<2,9>,P<3,4>,P<6,13>,P<8,14>,P<10,15>,P<11,12>>,
                          L<P<0,1>,P<2,3>,P<4,5>,P<6,8>,P<7,9>,P<10,11>,P<12,13>,P<14,15>>,
                          L<P<0,2>,P<1,3>,P<4,10>,P<5,11>,P<6,7>,P<8,9>,P<12,14>,P<13,15>>,
                          L<P<1,2>,P<3,12>,P<4,6>,P<5,7>,P<8,10>,P<9,11>,P<13,14>>,
                          L<P<1,4>,P<2,6>,P<5,8>,P<7,10>,P<9,13>,P<11,14>>,
                          L<P<2,4>,P<3,6>,P<9,12>,P<11,13>>,
                          L<P<3,5>,P<6,8>,P<7,9>,P<10,12>>,
                          L<P<3,4>,P<5,6>,P<7,8>,P<9,10>,P<11,12>>,
                          L<P<6,7>,P<8,9>>>,
                /* 17 */ N<L<P<0,11>,P<1,15>,P<2,10>,P<3,5>,P<4,6>,P<8,12>,P<9,16>,P<13,14>>,
                          L<P<0,6>,P<1,13>,P<2,8>,P<4,14>,P<5,15>,P<7,11>>,
                          L<P<0,8>,P<3,7>,P<4,9>,P<6,16>,P<10,11>,P<12,14>>,
                          L<P<0,2>,P<1,4>,P<5,6>,P<7,13>,P<8,9>,P<10,12>,P<11,14>,P<15,16>>,
                          L<P<0,3>,P<2,5>,P<6,11>,P<7,10>,P<9,13>,P<12,15>,P<14,16>>,
                          L<P<0,1>,P<3,4>,P<5,10>,P<6,9>,P<7,8>,P<11,15>,P<13,14>>,
                          L<P<1,2>,P<3,7>,P<4,8>,P<6,12>,P<11,13>,P<14,15>>,
                          L<P<1,3>,P<2,7>,P<4,5>,P<9,11>,P<10,12>,P<13,14>>,
                          L<P<2,3>,P<4,6>,P<5,7>,P<8,10>>,
                          L<P<3,4>,P<6,8>,P<7,9>,P<10,12>>,
                          L<P<5,6>,P<7,8>,P<9,10>,P<11,12>>,
                          L<P<4,5>,P<6,7>,P<8,9>,P<10,11>,P<12,13>>>,
                /* 18 */ N<L<P<0,1>,P<2,3>,P<4,5>,P<6,7>,P<8,9>,P<10,11>,P<12,13>,P<14,15>,P<16,17>>,
                          L<P<1,5>,P<2,6>,P<3,7>,P<4,10>,P<8,16>,P<9,17>,P<12,14>,P<13,15>>,
                          L<P<0,8>,P<1,10>,P<2,12>,P<3,14>,P<6,13>,P<7,15>,P<9,16>,P<11,17>>,
                          L<P<0,4>,P<1,9>,P<5,17>,P<8,11>,P<10,16>>,
                          L<P<0,2>,P<1,6>,P<4,10>,P<5,9>,P<14,16>,P<15,17>>,
                          L<P<1,2>,P<3,10>,P<4,12>,P<5,7>,P<6,14>,P<9,13>,P<15,16>>,
                          L<P<3,8>,P<5,12>,P<7,11>,P<9,10>>,
                          L<P<3,4>,P<6,8>,P<7,14>,P<9,12>,P<11,13>>,
                          L<P<1,3>,P<2,4>,P<7,9>,P<8,12>,P<11,15>,P<13,16>>,
                          L<P<2,3>,P<4,5>,P<6,7>,P<10,11>,P<12,14>,P<13,15>>,
                          L<P<4,6>,P<5,8>,P<9,10>,P<11,14>>,
                          L<P<3,4>,P<5,7>,P<8,9>,P<10,12>,P<13,14>>,
                          L<P<5,6>,P<7,8>,P<9,10>,P<11,12>>>,
                /* 19 */ N<L<P<0,12>,P<1,4>,P<2,8>,P<3,5>,P<6,17>,P<7,11>,P<9,14>,P<10,13>,P<15,16>>,
                          L<P<0,2>,P<1,7>,P<3,6>,P<4,11>,P<5,17>,P<8,12>,P<10,15>,P<13,16>,P<14,18>>,
                          L<P<3,10>,P<4,14>,P<5,15>,P<6,13>,P<7,9>,P<11,17>,P<16,18>>,
                          L<P<0,7>,P<1,10>,P<4,6>,P<9,15>,P<11,16>,P<12,17>,P<13,14>>,
                          L<P<0,3>,P<2,6>,P<5,7>,P<8,11>,P<12,16>>,
                          L<P<1,8>,P<2,9>,P<3,4>,P<6,15>,P<7,13>,P<10,11>,P<12,18>>,
                          L<P<1,3>,P<2,5>,P<6,9>,P<7,12>,P<8,10>,P<11,14>,P<17,18>>,
                          L<P<0,1>,P<2,3>,P<4,8>,P<6,10>,P<9,12>,P<14,15>,P<16,17>>,
                          L<P<1,2>,P<5,8>,P<6,7>,P<9,11>,P<10,13>,P<14,16>,P<15,17>>,
                          L<P<3,6>,P<4,5>,P<7,9>,P<8,10>,P<11,12>,P<13,14>,P<15,16>>,
                          L<P<3,4>,P<5,6>,P<7,8>,P<9,10>,P<11,13>,P<12,14>>,
                          L<P<2,3>,P<4,5>,P<6,7>,P<8,9>,P<10,11>,P<12,13>,P<14,15>>>,
                /* 20 */ N<L<P<0,3>,P<1,7>,P<2,5>,P<4,8>,P<6,9>,P<10,13>,P<11,15>,P<12,18>,P<14,17>,P<16,19>>,
                          L<P<0,14>,P<1,11>,P<2,16>,P<3,17>,P<4,12>,P<5,19>,P<6,10>,P<7,15>,P<8,18>,P<9,13>>,
                          L<P<0,4>,P<1,2>,P<3,8>,P<5,7>,P<11,16>,P<12,14>,P<15,19>,P<17,18>>,
                          L<P<1,6>,P<2,12>,P<3,5>,P<4,11>,P<7,17>,P<8,15>,P<13,18>,P<14,16>>,
                          L<P<0,1>,P<2,6>,P<7,10>,P<9,12>,P<13,17>,P<18,19>>,
                          L<P<1,6>,P<5,9>,P<7,11>,P<8,12>,P<10,14>,P<13,18>>,
                          L<P<3,5>,P<4,7>,P<8,10>,P<9,11>,P<12,15>,P<14,16>>,
                          L<P<1,3>,P<2,4>,P<5,7>,P<6,10>,P<9,13>,P<12,14>,P<15,17>,P<16,18>>,
                          L<P<1,2>,P<3,4>,P<6,7>,P<8,9>,P<10,11>,P<12,13>,P<15,16>,P<17,18>>,
                          L<P<2,3>,P<4,6>,P<5,8>,P<7,9>,P<10,12>,P<11,14>,P<13,15>,P<16,17>>,
                          L<P<4,5>,P<6,8>,P<7,10>,P<9,12>,P<11,13>,P<14,15>>,
                          L<P<3,4>,P<5,6>,P<7,8>,P<9,10>,P<11,12>,P<13,14>,P<15,16>>>,
                /* 21 */ N<L<P<0,7>,P<1,10>,P<3,5>,P<4,8>,P<6,13>,P<9,19>,P<11,14>,P<12,17>,P<15,16>,P<18,20>>,
                          L<P<0,11>,P<1,15>,P<2,12>,P<3,4>,P<5,8>,P<6,9>,P<7,14>,P<10,16>,P<13,19>,P<17,20>>,
                          L<P<0,6>,P<1,3>,P<2,18>,P<4,15>,P<5,10>,P<8,16>,P<11,17>,P<12,13>,P<14,20>>,
                          L<P<2,6>,P<5,12>,P<7,18>,P<8,14>,P<9,11>,P<10,17>,P<13,19>,P<16,20>>,
                          L<P<1,2>,P<4,7>,P<5,9>,P<6,17>,P<10,13>,P<11,12>,P<14,19>,P<15,18>>,
                          L<P<0,2>,P<3,6>,P<4,5>,P<7,10>,P<8,11>,P<9,15>,P<12,16>,P<13,18>,P<14,17>,P<19,20>>,
                          L<P<0,1>,P<2,3>,P<5,9>,P<6,12>,P<7,8>,P<11,14>,P<13,15>,P<16,19>,P<17,18>>,
                          L<P<1,2>,P<3,9>,P<6,13>,P<10,11>,P<12,15>,P<16,17>,P<18,19>>,
                          L<P<1,4>,P<2,5>,P<3,7>,P<6,10>,P<8,9>,P<11,12>,P<13,14>,P<17,18>>,
                          L<P<2,4>,P<5,6>,P<7,8>,P<9,11>,P<10,13>,P<12,15>,P<14,16>>,
                          L<P<3,4>,P<5,7>,P<6,8>,P<9,10>,P<11,13>,P<12,14>,P<15,16>>,
                          L<P<4,5>,P<6,7>,P<8,9>,P<10,11>,P<12,13>,P<14,15>,P<16,17>>>,
                /* 22 */ N<L<P<0,1>,P<2,3>,P<4,5>,P<6,7>,P<8,9>,P<10,11>,P<12,13>,P<14,15>,P<16,17>,P<18,19>,P<20,21>>,
                          L<P<0,12>,P<1,13>,P<2,6>,P<3,7>,P<4,10>,P<8,20>,P<9,21>,P<11,17>,P<14,18>,P<15,19>>,
                          L<P<0,2>,P<1,6>,P<3,12>,P<4,16>,P<5,17>,P<7,13>,P<8,14>,P<9,18>,P<15,20>,P<19,21>>,
                          L<P<0,8>,P<1,15>,P<2,14>,P<3,9>,P<5,11>,P<6,20>,P<7,19>,P<10,16>,P<12,18>,P<13,21>>,
                          L<P<0,4>,P<1,10>,P<3,8>,P<5,9>,P<7,14>,P<11,20>,P<12,16>,P<13,18>,P<17,21>>,
                          L<P<1,3>,P<2,5>,P<4,8>,P<6,9>,P<7,10>,P<11,14>,P<12,15>,P<13,17>,P<16,19>,P<18,20>>,
                          L<P<2,4>,P<3,12>,P<5,8>,P<6,11>,P<9,18>,P<10,15>,P<13,16>,P<17,19>>,
                          L<P<1,2>,P<3,4>,P<5,7>,P<6,12>,P<8,11>,P<9,15>,P<10,13>,P<14,16>,P<17,18>,P<19,20>>,
                          L<P<2,3>,P<4,5>,P<7,12>,P<8,10>,P<9,14>,P<11,13>,P<16,17>,P<18,19>>,
                          L<P<4,6>,P<5,8>,P<9,11>,P<10,12>,P<13,16>,P<15,17>>,
                          L<P<3,4>,P<6,7>,P<9,10>,P<11,12>,P<14,15>,P<17,18>>,
                          L<P<5,6>,P<7,8>,P<10,11>,P<13,14>,P<15,16>>,
                          L<P<6,7>,P<8,9>,P<12,13>,P<14,15>>>,
                /* 23 */ N<L<P<0,20>,P<1,12>,P<2,16>,P<4,6>,P<5,10>,P<7,21>,P<8,14>,P<9,15>,P<11,22>,P<13,18>,P<17,19>>,
                          L<P<0,3>,P<1,11>,P<2,7>,P<4,17>,P<5,13>,P<6,19>,P<8,9>,P<10,18>,P<12,22>,P<14,15>,P<16,21>>,
                          L<P<0,1>,P<2,4>,P<3,12>,P<5,8>,P<6,9>,P<7,10>,P<11,20>,P<13,16>,P<14,17>,P<15,18>,P<19,21>>,
                          L<P<2,5>,P<4,8>,P<6,11>,P<7,14>,P<9,16>,P<12,17>,P<15,19>,P<18,21>>,
                          L<P<1,8>,P<3,14>,P<4,7>,P<9,20>,P<10,12>,P<11,13>,P<15,22>,P<16,19>>,
                          L<P<0,7>,P<1,5>,P<3,4>,P<6,11>,P<8,15>,P<9,14>,P<10,13>,P<12,17>,P<18,22>,P<19,20>>,
                          L<P<0,2>,P<1,6>,P<4,7>,P<5,9>,P<8,10>,P<13,15>,P<14,18>,P<16,19>,P<17,22>,P<20,21>>,
                          L<P<2,3>,P<4,5>,P<6,8>,P<7,9>,P<10,11>,P<12,13>,P<14,16>,P<15,17>,P<18,19>,P<21,22>>,
                          L<P<1,2>,P<3,6>,P<4,10>,P<7,8>,P<9,11>,P<12,14>,P<13,19>,P<15,16>,P<17,20>>,
                          L<P<2,3>,P<5,10>,P<6,7>,P<8,9>,P<13,18>,P<14,15>,P<16,17>,P<20,21>>,
                          L<P<3,4>,P<5,7>,P<10,12>,P<11,13>,P<16,18>,P<19,20>>,
                          L<P<4,6>,P<8,10>,P<9,12>,P<11,14>,P<13,15>,P<17,19>>,
                          L<P<5,6>,P<7,8>,P<9,10>,P<11,12>,P<13,14>,P<15,16>,P<17,18>>>,
                /* 24 */ N<L<P<0,20>,P<1,12>,P<2,16>,P<3,23>,P<4,6>,P<5,10>,P<7,21>,P<8,14>,P<9,15>,P<11,22>,P<13,18>,P<17,19>>,
                          L<P<0,3>,P<1,11>,P<2,7>,P<4,17>,P<5,13>,P<6,19>,P<8,9>,P<10,18>,P<12,22>,P<14,15>,P<16,21>,P<20,23>>,
                          L<P<0,1>,P<2,4>,P<3,12>,P<5,8>,P<6,9>,P<7,10>,P<11,20>,P<13,16>,P<14,17>,P<15,18>,P<19,21>,P<22,23>>,
                          L<P<2,5>,P<4,8>,P<6,11>,P<7,14>,P<9,16>,P<12,17>,P<15,19>,P<18,21>>,
                          L<P<1,8>,P<3,14>,P<4,7>,P<9,20>,P<10,12>,P<11,13>,P<15,22>,P<16,19>>,
                          L<P<0,7>,P<1,5>,P<3,4>,P<6,11>,P<8,15>,P<9,14>,P<10,13>,P<12,17>,P<16,23>,P<18,22>,P<19,20>>,
                          L<P<0,2>,P<1,6>,P<4,7>,P<5,9>,P<8,10>,P<13,15>,P<14,18>,P<16,19>,P<17,22>,P<21,23>>,
                          L<P<2,3>,P<4,5>,P<6,8>,P<7,9>,P<10,11>,P<12,13>,P<14,16>,P<15,17>,P<18,19>,P<20,21>>,
                          L<P<1,2>,P<3,6>,P<4,10>,P<7,8>,P<9,11>,P<12,14>,P<13,19>,P<15,16>,P<17,20>,P<21,22>>,
                          L<P<2,3>,P<5,10>,P<6,7>,P<8,9>,P<13,18>,P<14,15>,P<16,17>,P<20,21>>,
                          L<P<3,4>,P<5,7>,P<10,12>,P<11,13>,P<16,18>,P<19,20>>,
                          L<P<4,6>,P<8,10>,P<9,12>,P<11,14>,P<13,15>,P<17,19>>,
                          L<P<5,6>,P<7,8>,P<9,10>,P<11,12>,P<13,14>,P<15,16>,P<17,18>>>,
                /* 25 */ N<L<P<0,2>,P<1,8>,P<3,18>,P<4,17>,P<5,20>,P<6,19>,P<7,9>,P<10,11>,P<12,13>,P<14,16>,P<15,22>,P<21,23>>,
                          L<P<0,3>,P<1,15>,P<2,18>,P<4,12>,P<5,21>,P<6,10>,P<7,14>,P<8,22>,P<9,16>,P<11,19>,P<13,17>,P<20,23>>,
                          L<P<0,4>,P<1,7>,P<2,13>,P<3,12>,P<5,6>,P<8,14>,P<9,15>,P<10,21>,P<11,20>,P<16,22>,P<17,18>,P<19,23>>,
                          L<P<0,5>,P<2,11>,P<3,6>,P<4,10>,P<7,16>,P<8,9>,P<12,21>,P<13,19>,P<14,15>,P<17,20>,P<18,23>>,
                          L<P<2,7>,P<6,9>,P<8,11>,P<14,24>,P<18,21>>,
                          L<P<3,8>,P<7,10>,P<11,12>,P<13,14>,P<15,21>,P<18,20>,P<22,24>>,
                          L<P<4,13>,P<10,16>,P<11,15>,P<18,24>,P<19,22>>,
                          L<P<1,4>,P<8,11>,P<9,19>,P<13,17>,P<14,18>,P<16,20>,P<23,24>>,
                          L<P<0,1>,P<4,5>,P<6,13>,P<9,14>,P<10,17>,P<12,16>,P<18,19>,P<20,21>,P<22,23>>,
                          L<P<2,6>,P<3,4>,P<5,13>,P<7,9>,P<12,18>,P<15,17>,P<16,19>,P<20,22>,P<21,23>>,
                          L<P<1,2>,P<5,8>,P<6,7>,P<9,10>,P<11,13>,P<14,15>,P<17,20>,P<21,22>>,
                          L<P<1,3>,P<2,4>,P<5,6>,P<7,11>,P<8,9>,P<10,13>,P<12,14>,P<15,16>,P<17,18>,P<19,20>>,
                          L<P<2,3>,P<4,8>,P<6,7>,P<9,12>,P<10,11>,P<13,14>,P<15,17>,P<16,18>,P<20,21>>,
                          L<P<3,5>,P<4,6>,P<7,8>,P<9,10>,P<11,12>,P<13,15>,P<14,17>,P<16,19>>,
                          L<P<4,5>,P<6,7>,P<8,9>,P<10,11>,P<12,13>,P<14,15>,P<16,17>,P<18,19>>>,
                /* 26 */ N<L<P<0,25>,P<1,3>,P<2,9>,P<4,19>,P<5,18>,P<6,21>,P<7,20>,P<8,10>,P<11,12>,P<13,14>,P<15,17>,P<16,23>,P<22,24>>,
                          L<P<1,4>,P<2,16>,P<3,19>,P<5,13>,P<6,22>,P<7,11>,P<8,15>,P<9,23>,P<10,17>,P<12,20>,P<14,18>,P<21,24>>,
                          L<P<1,5>,P<2,8>,P<3,14>,P<4,13>,P<6,7>,P<9,15>,P<10,16>,P<11,22>,P<12,21>,P<17,23>,P<18,19>,P<20,24>>,
                          L<P<0,10>,P<1,6>,P<3,7>,P<4,11>,P<5,12>,P<13,20>,P<14,21>,P<15,25>,P<18,22>,P<19,24>>,
                          L<P<0,4>,P<8,10>,P<12,13>,P<15,17>,P<21,25>>,
                          L<P<0,2>,P<4,8>,P<10,12>,P<13,15>,P<17,21>,P<23,25>>,
                          L<P<0,1>,P<2,3>,P<4,5>,P<8,14>,P<9,13>,P<11,17>,P<12,16>,P<20,21>,P<22,23>,P<24,25>>,
                          L<P<1,4>,P<3,10>,P<6,9>,P<7,13>,P<8,11>,P<12,18>,P<14,17>,P<15,22>,P<16,19>,P<21,24>>,
                          L<P<2,6>,P<3,8>,P<5,7>,P<9,12>,P<13,16>,P<17,22>,P<18,20>,P<19,23>>,
                          L<P<1,2>,P<4,6>,P<5,9>,P<7,10>,P<11,12>,P<13,14>,P<15,18>,P<16,20>,P<19,21>,P<23,24>>,
                          L<P<2,4>,P<3,5>,P<7,13>,P<8,9>,P<10,14>,P<11,15>,P<12,18>,P<16,17>,P<20,22>,P<21,23>>,
                          L<P<3,4>,P<6,9>,P<7,11>,P<10,12>,P<13,15>,P<14,18>,P<16,19>,P<21,22>>,
                          L<P<5,7>,P<6,8>,P<9,13>,P<10,11>,P<12,16>,P<14,15>,P<17,19>,P<18,20>>,
                          L<P<5,6>,P<7,8>,P<9,10>,P<11,13>,P<12,14>,P<15,16>,P<17,18>,P<19,20>>,
                          L<P<4,5>,P<6,7>,P<8,9>,P<10,11>,P<12,13>,P<14,15>,P<16,17>,P<18,19>,P<20,21>>>,
                /* 27 */ N<L<P<0,9>,P<1,6>,P<2,4>,P<3,7>,P<5,8>,P<11,24>,P<12,23>,P<13,26>,P<14,25>,P<15,19>,P<16,17>,P<18,22>,P<20,21>>,
                          L<P<0,1>,P<3,5>,P<4,10>,P<6,9>,P<7,8>,P<11,16>,P<12,18>,P<13,20>,P<14,15>,P<17,24>,P<19,25>,P<21,26>,P<22,23>>,
                          L<P<1,3>,P<2,5>,P<4,7>,P<8,10>,P<11,12>,P<13,14>,P<15,16>,P<17,19>,P<18,20>,P<21,22>,P<23,24>,P<25,26>>,
                          L<P<0,4>,P<1,2>,P<3,7>,P<5,9>,P<6,8>,P<11,13>,P<12,14>,P<15,21>,P<16,22>,P<17,18>,P<19,20>,P<23,25>,P<24,26>>,
                          L<P<0,1>,P<2,6>,P<4,5>,P<7,8>,P<9,10>,P<12,13>,P<14,23>,P<15,17>,P<16,18>,P<19,21>,P<20,22>,P<24,25>>,
                          L<P<0,11>,P<2,4>,P<3,6>,P<5,7>,P<8,9>,P<12,15>,P<13,17>,P<16,19>,P<18,21>,P<20,24>,P<22,25>>,
                          L<P<1,2>,P<3,4>,P<5,6>,P<7,8>,P<13,15>,P<14,17>,P<20,23>,P<22,24>>,
                          L<P<1,12>,P<2,3>,P<4,5>,P<6,7>,P<14,16>,P<17,19>,P<18,20>,P<21,23>>,
                          L<P<2,13>,P<14,15>,P<16,17>,P<18,19>,P<20,21>,P<22,23>>,
                          L<P<3,14>,P<4,15>,P<5,16>,P<10,21>,P<17,18>,P<19,20>>,
                          L<P<6,17>,P<7,18>,P<8,19>,P<9,20>,P<10,13>,P<14,22>,P<15,23>,P<16,24>>,
                          L<P<6,10>,P<7,14>,P<8,11>,P<9,12>,P<17,25>,P<18,26>,P<19,23>,P<20,24>>,
                          L<P<4,8>,P<5,9>,P<11,15>,P<12,16>,P<13,17>,P<18,22>,P<21,25>,P<24,26>>,
                          L<P<2,4>,P<3,5>,P<6,8>,P<7,9>,P<10,11>,P<12,14>,P<13,15>,P<16,18>,P<17,19>,P<20,22>,P<21,23>,P<25,26>>,
                          L<P<1,2>,P<3,4>,P<5,6>,P<7,8>,P<9,10>,P<11,12>,P<13,14>,P<15,16>,P<17,18>,P<19,20>,P<21,22>,P<23,24>>>,
                /* 28 */ N<L<P<0,9>,P<1,20>,P<2,21>,P<3,22>,P<4,19>,P<5,24>,P<6,25>,P<7,26>,P<8,23>,P<10,15>,P<11,13>,P<12,17>,P<14,16>,P<18,27>>,
                          L<P<0,18>,P<1,7>,P<2,6>,P<3,5>,P<4,8>,P<9,27>,P<10,12>,P<11,14>,P<13,16>,P<15,17>,P<19,23>,P<20,26>,P<21,25>,P<22,24>>,
                          L<P<1,2>,P<3,4>,P<5,19>,P<6,20>,P<7,21>,P<8,22>,P<9,18>,P<10,11>,P<12,14>,P<13,15>,P<16,17>,P<23,24>,P<25,26>>,
                          L<P<0,3>,P<1,10>,P<5,8>,P<6,7>,P<11,13>,P<14,16>,P<17,26>,P<19,22>,P<20,21>,P<24,27>>,
                          L<P<0,1>,P<2,7>,P<3,10>,P<4,8>,P<12,13>,P<14,15>,P<17,24>,P<19,23>,P<20,25>,P<26,27>>,
                          L<P<1,3>,P<2,6>,P<4,5>,P<7,19>,P<8,20>,P<11,12>,P<13,14>,P<15,16>,P<21,25>,P<22,23>,P<24,26>>,
                          L<P<2,4>,P<5,12>,P<7,8>,P<9,11>,P<10,14>,P<13,17>,P<15,22>,P<16,18>,P<19,20>,P<23,25>>,
                          L<P<2,9>,P<4,11>,P<5,6>,P<7,13>,P<8,10>,P<14,20>,P<16,23>,P<17,19>,P<18,25>,P<21,22>>,
                          L<P<1,2>,P<3,16>,P<4,9>,P<6,12>,P<10,14>,P<11,24>,P<13,17>,P<15,21>,P<18,23>,P<25,26>>,
                          L<P<2,8>,P<3,5>,P<4,7>,P<6,16>,P<9,15>,P<11,21>,P<12,18>,P<19,25>,P<20,23>,P<22,24>>,
                          L<P<2,3>,P<5,8>,P<7,9>,P<11,15>,P<12,16>,P<18,20>,P<19,22>,P<24,25>>,
                          L<P<6,8>,P<10,12>,P<11,13>,P<14,16>,P<15,17>,P<19,21>>,
                          L<P<5,6>,P<8,10>,P<9,11>,P<12,13>,P<14,15>,P<16,18>,P<17,19>,P<21,22>>,
                          L<P<4,5>,P<6,7>,P<8,9>,P<10,11>,P<12,14>,P<13,15>,P<16,17>,P<18,19>,P<20,21>,P<22,23>>,
                          L<P<3,4>,P<5,6>,P<7,8>,P<9,10>,P<11,12>,P<13,14>,P<15,16>,P<17,18>,P<19,20>,P<21,22>,P<23,24>>>,
                /* 29 */ N<L<P<0,12>,P<1,10>,P<2,9>,P<3,7>,P<5,11>,P<6,8>,P<13,26>,P<14,25>,P<15,28>,P<16,27>,P<17,21>,P<18,19>,P<20,24>,P<22,23>>,
                          L<P<1,6>,P<2,3>,P<4,11>,P<7,9>,P<8,10>,P<13,18>,P<14,20>,P<15,22>,P<16,17>,P<19,26>,P<21,27>,P<23,28>,P<24,25>>,
                          L<P<0,4>,P<1,2>,P<3,6>,P<7,8>,P<9,10>,P<11,12>,P<13,14>,P<15,16>,P<17,18>,P<19,21>,P<20,22>,P<23,24>,P<25,26>,P<27,28>>,
                          L<P<4,6>,P<5,9>,P<8,11>,P<10,12>,P<13,15>,P<14,16>,P<17,23>,P<18,24>,P<19,20>,P<21,22>,P<25,27>,P<26,28>>,
                          L<P<0,5>,P<3,8>,P<4,7>,P<6,11>,P<9,10>,P<14,15>,P<16,25>,P<17,19>,P<18,20>,P<21,23>,P<22,24>,P<26,27>>,
                          L<P<0,1>,P<2,5>,P<6,9>,P<7,8>,P<10,11>,P<14,17>,P<15,19>,P<18,21>,P<20,23>,P<22,26>,P<24,27>>,
                          L<P<0,13>,P<1,3>,P<2,4>,P<5,6>,P<9,10>,P<15,17>,P<16,19>,P<22,25>,P<24,26>>,
                          L<P<1,2>,P<3,4>,P<5,7>,P<6,8>,P<16,18>,P<19,21>,P<20,22>,P<23,25>>,
                          L<P<1,14>,P<2,3>,P<4,5>,P<6,7>,P<8,9>,P<16,17>,P<18,19>,P<20,21>,P<22,23>,P<24,25>>,
                          L<P<2,15>,P<3,4>,P<5,6>,P<10,23>,P<11,24>,P<12,25>,P<19,20>,P<21,22>>,
                          L<P<3,16>,P<4,17>,P<5,18>,P<6,19>,P<7,20>,P<8,21>,P<9,22>,P<10,15>>,
                          L<P<6,10>,P<8,13>,P<9,14>,P<11,16>,P<12,17>,P<18,26>,P<19,27>,P<20,28>>,
                          L<P<4,8>,P<5,9>,P<7,11>,P<12,13>,P<14,18>,P<15,19>,P<16,20>,P<17,21>,P<22,26>,P<23,27>,P<24,28>>,
                          L<P<2,4>,P<3,5>,P<6,8>,P<7,9>,P<10,12>,P<11,14>,P<13,15>,P<16,18>,P<17,19>,P<20,22>,P<21,23>,P<24,26>,P<25,27>>,
                          L<P<1,2>,P<3,4>,P<5,6>,P<7,8>,P<9,10>,P<11,12>,P<13,14>,P<15,16>,P<17,18>,P<19,20>,P<21,22>,P<23,24>,P<25,26>,P<27,28>>>,
                /* 30 */ N<L<P<1,2>,P<3,10>,P<4,14>,P<5,8>,P<6,13>,P<7,12>,P<9,11>,P<16,17>,P<18,25>,P<19,29>,P<20,23>,P<21,28>,P<22,27>,P<24,26>>,
                          L<P<0,14>,P<1,5>,P<2,8>,P<3,7>,P<6,9>,P<10,12>,P<11,13>,P<15,29>,P<16,20>,P<17,23>,P<18,22>,P<21,24>,P<25,27>,P<26,28>>,
                          L<P<0,7>,P<1,6>,P<2,9>,P<4,10>,P<5,11>,P<8,13>,P<12,14>,P<15,22>,P<16,21>,P<17,24>,P<19,25>,P<20,26>,P<23,28>,P<27,29>>,
                          L<P<0,6>,P<2,4>,P<3,5>,P<7,11>,P<8,10>,P<9,12>,P<13,14>,P<15,21>,P<17,19>,P<18,20>,P<22,26>,P<23,25>,P<24,27>,P<28,29>>,
                          L<P<0,3>,P<1,2>,P<4,7>,P<5,9>,P<6,8>,P<10,11>,P<12,13>,P<14,29>,P<15,18>,P<16,17>,P<19,22>,P<20,24>,P<21,23>,P<25,26>,P<27,28>>,
                          L<P<0,1>,P<2,3>,P<4,6>,P<7,9>,P<10,12>,P<11,13>,P<15,16>,P<17,18>,P<19,21>,P<22,24>,P<25,27>,P<26,28>>,
                          L<P<0,15>,P<1,2>,P<3,5>,P<8,10>,P<11,12>,P<13,28>,P<16,17>,P<18,20>,P<23,25>,P<26,27>>,
                          L<P<1,16>,P<3,4>,P<5,6>,P<7,8>,P<9,10>,P<12,27>,P<18,19>,P<20,21>,P<22,23>,P<24,25>>,
                          L<P<2,3>,P<4,5>,P<6,7>,P<8,9>,P<10,11>,P<17,18>,P<19,20>,P<21,22>,P<23,24>,P<25,26>>,
                          L<P<2,17>,P<3,18>,P<4,19>,P<5,6>,P<7,8>,P<9,24>,P<10,25>,P<11,26>,P<20,21>,P<22,23>>,
                          L<P<5,20>,P<6,21>,P<7,22>,P<8,23>,P<9,16>,P<10,17>,P<11,18>,P<12,19>>,
                          L<P<5,9>,P<6,10>,P<7,11>,P<8,15>,P<13,20>,P<14,21>,P<18,22>,P<19,23>>,
                          L<P<3,5>,P<4,8>,P<7,9>,P<12,15>,P<13,16>,P<14,17>,P<20,24>,P<21,25>>,
                          L<P<2,4>,P<6,8>,P<10,12>,P<11,13>,P<14,15>,P<16,18>,P<17,19>,P<20,22>,P<21,23>,P<24,26>,P<25,27>>,
                          L<P<1,2>,P<3,4>,P<5,6>,P<7,8>,P<9,10>,P<11,12>,P<13,14>,P<15,16>,P<17,18>,P<19,20>,P<21,22>,P<23,24>,P<25,26>,P<27,28>>>,
                /* 31 */ N<L<P<0,1>,P<2,3>,P<4,5>,P<6,7>,P<8,9>,P<10,11>,P<12,13>,P<14,15>,P<16,17>,P<18,19>,P<20,21>,P<22,23>,P<24,25>,P<26,27>,P<28,29>>,
                          L<P<0,2>,P<1,3>,P<4,6>,P<5,7>,P<8,10>,P<9,11>,P<12,14>,P<13,15>,P<16,18>,P<17,19>,P<20,22>,P<21,23>,P<24,26>,P<25,27>,P<28,30>>,
                          L<P<0,4>,P<1,5>,P<2,6>,P<3,7>,P<8,12>,P<9,13>,P<10,14>,P<11,15>,P<16,20>,P<17,21>,P<18,22>,P<19,23>,P<24,28>,P<25,29>,P<26,30>>,
                          L<P<0,8>,P<1,9>,P<2,10>,P<3,11>,P<4,12>,P<5,13>,P<6,14>,P<7,15>,P<16,24>,P<17,25>,P<18,26>,P<19,27>,P<20,28>,P<21,29>,P<22,30>>,
                          L<P<0,16>,P<1,8>,P<2,4>,P<3,12>,P<5,10>,P<6,9>,P<7,14>,P<11,13>,P<17,24>,P<18,20>,P<19,28>,P<21,26>,P<22,25>,P<23,30>,P<27,29>>,
                          L<P<1,2>,P<3,5>,P<4,8>,P<6,22>,P<7,11>,P<9,25>,P<10,12>,P<13,14>,P<17,18>,P<19,21>,P<20,24>,P<23,27>,P<26,28>,P<29,30>>,
                          L<P<1,17>,P<2,18>,P<3,19>,P<4,20>,P<5,10>,P<7,23>,P<8,24>,P<11,27>,P<12,28>,P<13,29>,P<14,30>,P<21,26>>,
                          L<P<3,17>,P<4,16>,P<5,21>,P<6,18>,P<7,9>,P<8,20>,P<10,26>,P<11,23>,P<13,25>,P<14,28>,P<15,27>,P<22,24>>,
                          L<P<1,4>,P<3,8>,P<5,16>,P<7,17>,P<9,21>,P<10,22>,P<11,19>,P<12,20>,P<14,24>,P<15,26>,P<23,28>,P<27,30>>,
                          L<P<2,5>,P<7,8>,P<9,18>,P<11,17>,P<12,16>,P<13,22>,P<14,20>,P<15,19>,P<23,24>,P<26,29>>,
                          L<P<2,4>,P<6,12>,P<9,16>,P<10,11>,P<13,17>,P<14,18>,P<15,22>,P<19,25>,P<20,21>,P<27,29>>,
                          L<P<5,6>,P<8,12>,P<9,10>,P<11,13>,P<14,16>,P<15,17>,P<18,20>,P<19,23>,P<21,22>,P<25,26>>,
                          L<P<3,5>,P<6,7>,P<8,9>,P<10,12>,P<11,14>,P<13,16>,P<15,18>,P<17,20>,P<19,21>,P<22,23>,P<24,25>,P<26,28>>,
                          L<P<3,4>,P<5,6>,P<7,8>,P<9,10>,P<11,12>,P<13,14>,P<15,16>,P<17,18>,P<19,20>,P<21,22>,P<23,24>,P<25,26>,P<27,28>>>,
                /* 32 */ N<L<P<0,1>,P<2,3>,P<4,5>,P<6,7>,P<8,9>,P<10,11>,P<12,13>,P<14,15>,P<16,17>,P<18,19>,P<20,21>,P<22,23>,P<24,25>,P<26,27>,P<28,29>,P<30,31>>,
                          L<P<0,2>,P<1,3>,P<4,6>,P<5,7>,P<8,10>,P<9,11>,P<12,14>,P<13,15>,P<16,18>,P<17,19>,P<20,22>,P<21,23>,P<24,26>,P<25,27>,P<28,30>,P<29,31>>,
                          L<P<0,4>,P<1,5>,P<2,6>,P<3,7>,P<8,12>,P<9,13>,P<10,14>,P<11,15>,P<16,20>,P<17,21>,P<18,22>,P<19,23>,P<24,28>,P<25,29>,P<26,30>,P<27,31>>,
                          L<P<0,8>,P<1,9>,P<2,10>,P<3,11>,P<4,12>,P<5,13>,P<6,14>,P<7,15>,P<16,24>,P<17,25>,P<18,26>,P<19,27>,P<20,28>,P<21,29>,P<22,30>,P<23,31>>,
                          L<P<0,16>,P<1,8>,P<2,4>,P<3,12>,P<5,10>,P<6,9>,P<7,14>,P<11,13>,P<15,31>,P<17,24>,P<18,20>,P<19,28>,P<21,26>,P<22,25>,P<23,30>,P<27,29>>,
                          L<P<1,2>,P<3,5>,P<4,8>,P<6,22>,P<7,11>,P<9,25>,P<10,12>,P<13,14>,P<17,18>,P<19,21>,P<20,24>,P<23,27>,P<26,28>,P<29,30>>,
                          L<P<1,17>,P<2,18>,P<3,19>,P<4,20>,P<5,10>,P<7,23>,P<8,24>,P<11,27>,P<12,28>,P<13,29>,P<14,30>,P<21,26>>,
                          L<P<3,17>,P<4,16>,P<5,21>,P<6,18>,P<7,9>,P<8,20>,P<10,26>,P<11,23>,P<13,25>,P<14,28>,P<15,27>,P<22,24>>,
                          L<P<1,4>,P<3,8>,P<5,16>,P<7,17>,P<9,21>,P<10,22>,P<11,19>,P<12,20>,P<14,24>,P<15,26>,P<23,28>,P<27,30>>,
                          L<P<2,5>,P<7,8>,P<9,18>,P<11,17>,P<12,16>,P<13,22>,P<14,20>,P<15,19>,P<23,24>,P<26,29>>,
                          L<P<2,4>,P<6,12>,P<9,16>,P<10,11>,P<13,17>,P<14,18>,P<15,22>,P<19,25>,P<20,21>,P<27,29>>,
                          L<P<5,6>,P<8,12>,P<9,10>,P<11,13>,P<14,16>,P<15,17>,P<18,20>,P<19,23>,P<21,22>,P<25,26>>,
                          L<P<3,5>,P<6,7>,P<8,9>,P<10,12>,P<11,14>,P<13,16>,P<15,18>,P<17,20>,P<19,21>,P<22,23>,P<24,25>,P<26,28>>,
                          L<P<3,4>,P<5,6>,P<7,8>,P<9,10>,P<11,12>,P<13,14>,P<15,16>,P<17,18>,P<19,20>,P<21,22>,P<23,24>,P<25,26>,P<27,28>>>
    >;
    // clang-format on
  }  // namespace sorter_hunter

  // Generates sorting networks which are either proven to be
  // size optimal (and are thus comprised of minimum number of compare
  // and swap operations) or are found by the "SorterHunter"
  // algorithm (https://github.com/bertdobbelaere/SorterHunter)
  template<std::size_t N>
  struct sorting_network<N, type::size_optimized_sort,
                         std::enable_if_t<(N < std::tuple_size<sorter_hunter::Networks>::value)>>
  {
    using type = std::tuple_element_t<N - 1, sorter_hunter::Networks>;
  };
}  // namespace quxflux::sorting_net

