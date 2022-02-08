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
#include <functional>
#include <iterator>
#include <utility>

#include <sorting_network_cpp/sorting_network_impl.h>

namespace quxflux::sorting_net
{
  template<std::size_t N, type NetworkType = type::bose_nelson_sort>
  struct sorting_network
  {
    static_assert(N > 0);

    template<typename It,
             typename CompareAndSwap = compare_and_swap<typename std::iterator_traits<It>::value_type,
                                                        std::less<typename std::iterator_traits<It>::value_type>>,
             typename = std::enable_if_t<detail::is_random_access_iterator_v<It>>>
    constexpr void operator()(const It begin, const CompareAndSwap cas = {}) const
    {
      if constexpr (N > 1)
        sorting_net::apply(begin, cas, typename detail::sorting_network<N, NetworkType>::type{});
    }
  };
}  // namespace quxflux::sorting_net
