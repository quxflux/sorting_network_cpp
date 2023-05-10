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
