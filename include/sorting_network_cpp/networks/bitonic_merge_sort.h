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
