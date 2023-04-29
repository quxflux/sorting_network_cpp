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
