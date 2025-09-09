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

#include "test_base.h"

#include <sorting_network_cpp/networks/insertion_sort.h>

namespace quxflux::sorting_net
{
  inline constexpr std::size_t max_insertion_sort_array_size_to_test =
#if defined(_MSC_VER)
    17  // if chosen too large, msbuild will fail with "fatal error C1202: recursive type or function dependency context
        // too complex"
#else
    max_array_size_to_test
#endif
    ;

  using specs = test_specs_for_network<type::insertion_sort, max_insertion_sort_array_size_to_test>;

  INSTANTIATE_TYPED_TEST_SUITE_P(sorting_network_test_insertion_sort, sorting_network_test, specs);

}  // namespace quxflux::sorting_net
