# sorting_network_cpp

sorting_network_cpp offers templated implementations of various types of [sorting networks](https://en.wikipedia.org/wiki/Sorting_network) which can drastically improve performance for small problem sizes of simple data compared to regular sorting algorithms (e.g. `std::sort`).

The following listing shows the execution time for different algorithms to sort one million arrays of each 32 random `float` values on an AMD Ryzen 7 2700x @3.70 GHz (compiled with clang 12.0.0 `-O3`):

| algorithm                   | time (ms) | speedup vs `std::sort` |
|-----------------------------|:---------:|:----------------------:|
| Batcher Odd Even Merge Sort |   74.10   |          8.4           |
| Bitonic Merge Sort          |   86.53   |          7.2           |
| Bose Nelson Sort            |   73.05   |          8.6           |
| Bubble Sort                 |  185.58   |          3.3           |
| Insertion Sort              |  189.93   |          3.3           |
| std::sort                   |  628.74   |           1            |

An interactive overview of performance measurements for the sorting networks on different data types and problem sizes can be found [here](https://raw.githack.com/quxflux/sorting_network_cpp/master/doc/data_explorer.htm).

## Usage

A single class `sorting_network` is available which encapsulates the implementation of the different sorting networks.

The following listing gives a few examples:

```cpp
#include <sorting_network_cpp/sorting_network.h>

#include <array>
#include <cstdint>

void example()
{
  using namespace quxflux::sorting_net;

  static constexpr std::size_t N = 16;

  {
    std::array<int, N> my_array;
    // fill array...

    sorting_network<N>{}(my_array.begin());
    // my_array is now sorted
  }

  {
    int data[N];
    // fill array...

    // raw pointers work as well...
    sorting_network<N>{}(data);
    // data is now sorted
  }

  {
    int data[N];

    // by default, std::less<T> is used as the comparator; custom comparators
    // may be passed as follows:

    // custom comparators
    sorting_network<N>{}(data, compare_and_swap<int, std::less<>>{});

    // function objects
    struct predicate
    {
      constexpr bool operator()(const int a, const int b) const { return a < b; }
    };

    sorting_network<N>{}(data, compare_and_swap<int, predicate>{});
  }
}
```

When no `type` is explicitly specified as template argument for `sorting_network`, the `type::bose_nelson_sort` is used.

Available network types ATM are:
* `insertion_sort`
* `bubble_sort`
* `bose_nelson_sort`
* `batcher_odd_even_merge_sort`
* `bitonic_merge_sort`
* `size_optimized_sort`

The following example shows how to specify a different `type` than the default one:

```cpp
quxflux::sorting_net::sorting_network<N, quxflux::sorting_net::type::bitonic_merge_sort>()(std::begin(data_to_be_sorted));
```
## Using custom compare and swap implementations

The compare and swap operation is the fundamental element a sorting network is composed of. The default implementation works well on scalar types. However, if you want to specify a custom implementation (e.g., when hardware intrinsics should be used), you may do this by providing a compare and swap functor to the `sorting_network::operator()` as in the following example:

```cpp
#include <sorting_network_cpp/sorting_network.h>

#include <array>

void example(std::array<float,3>& arr)
{
  quxflux::sorting_net::sorting_network<3>{}(arr.begin(), [](float& a, float& b){
    const auto b_cpy = b;
    b = std::max(a, b);
    a = std::min(a, b_cpy);
  });
}
```

## Single header implementation
A single header implementation is available which allows experimenting with the sorting networks on [godbolt](https://godbolt.org/z/69WMqMY3c).

## Requirements
A compiler with C++17 support

## Dependencies
For the bare sorting functionality, no dependencies are required. If you want to run the tests, the required dependencies will automatically be fetched via [CPM](https://github.com/cpm-cmake/CPM.cmake).

## Limitations
* Depending on the implementation of the comparator the performance advantage of a sorting net compared to a regular sorting algorithm (e.g. `std::sort`) may diminish or even result in worse performance. This can be seen in the [interactive benchmark results overview](https://raw.githack.com/quxflux/sorting_network_cpp/master/doc/data_explorer.htm) for the data type `Vec2i Z-order` which causes in most cases all variants of sorting networks being outperformed by `std::sort` (see [src/benchmark.cpp](src/benchmark.cpp) for the implementation of the aforementioned data type).
* msvc will fail to compile larger `insertion_sort` networks in certain (unknown) configurations with `fatal error C1202: recursive type or function dependency context too complex`

## Development notes
* A pre-push [githook](https://git-scm.com/docs/githooks) is available in [.githooks](./.githooks/) which will automatically create the single header implementation. 
To enable the hook execute 
`git config --local core.hooksPath .githooks` in the repository directory (python required)

## References / Acknowledgements
* ["A Sorting Problem"](https://dl.acm.org/doi/pdf/10.1145/321119.321126) by Bose et al.
* ["Sorting networks and their applications"](https://core.ac.uk/download/pdf/192393620.pdf) by Batcher
* [Vectorized/Static-Sort](https://github.com/Vectorized/Static-Sort) adaption for Bose-Nelson sort
* [HS Flensburg](https://www.inf.hs-flensburg.de/lang/algorithmen/sortieren/networks/oemen.htm) explanation for Batcher's odd-even mergesort
* [HS Flensburg](https://www.inf.hs-flensburg.de/lang/algorithmen/sortieren/bitonic/oddn.htm) explanation for bitonic sort
* [SortHunter](https://github.com/bertdobbelaere/SorterHunter) for size optimized sorting networks
* [Google Test](https://github.com/google/googletest) for testing
* [Chart.js](https://www.chartjs.org/) and [Papa Parse](https://www.papaparse.com/) for the visualization of benchmark results

## License
[GPLv3](LICENSE)
