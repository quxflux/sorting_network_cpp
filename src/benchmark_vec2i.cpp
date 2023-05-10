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

#include "util.h"

namespace quxflux
{
  namespace
  {
    static constexpr uint16_t expand_bits(uint16_t x) noexcept
    {
      x &= 0x000003ff;                   // x = ---- ---- ---- ---- ---- --98 7654 3210
      x = (x ^ (x << 16)) & 0x030000ff;  // x = ---- --98 ---- ---- ---- ---- 7654 3210
      x = (x ^ (x << 8)) & 0x0300f00f;   // x = ---- --98 ---- ---- 7654 ---- ---- 3210
      x = (x ^ (x << 4)) & 0x030c30c3;   // x = ---- --98 ---- 76-- --54 ---- 32-- --10
      x = (x ^ (x << 2)) & 0x09249249;   // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0

      return x;
    }

    static constexpr uint32_t generate_morton_code_2d(const vec2i& p) noexcept
    {
      return (expand_bits(p.data[0]) << 1 | expand_bits(p.data[1]));
    }
  }  // namespace

  constexpr bool operator<(const vec2i& lhs, const vec2i& rhs) noexcept
  {
    // A slightly more complex comparison operation:
    // compare the morton code of two points (by generating the
    // morton codes on the fly)

    return generate_morton_code_2d(lhs) < generate_morton_code_2d(rhs);
  }

  IMPL_BENCHMARK(vec2i);
}  // namespace quxflux
