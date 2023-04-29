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
