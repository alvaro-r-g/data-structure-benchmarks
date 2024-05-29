
# Data Structure Benchmarks

This repository adds structures with the following sycnhronization mechanisms to [mcas-benchmarks](https://github.com/kankava/mcas-benchmarks):

- [Flock](https://github.com/cmuparlay/flock)
- HTM (Hardware Transactional Memory)
- MCAS (Multi-address Compare-And-Swap)

Flock library source code was imported from [Verlib](https://github.com/cmuparlay/verlib/tree/main/include/flock) to allow scheduling with `pthreads`.

## Requirements

- Processor with Intel TSX extensions enabled, including support for [RTM](https://www.intel.com/content/www/us/en/docs/cpp-compiler/developer-guide-reference/2021-8/restricted-transactional-memory-overview.html) (needed for HTM structures).
- jemalloc-5.2.1 (needed for Flock): <https://github.com/jemalloc/jemalloc/releases/download/5.2.1/jemalloc-5.2.1.tar.bz2>
  - To install, extract and follow instructions in the INSTALL text file
  - Then run ```sudo apt install libjemalloc-dev```
  - If you install jemalloc locally instead, change the path in CMakeLists.txt (`find_library(JEMALLOC_LIB jemalloc PATHS ~/local/jemalloc/lib/)`) to your local path
- python3 and matplotlib

## Compilation

```sh
# From the project directory
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DHTM_BACKOFF=0 ..
cmake --build .
```

`HTM_BACKOFF` can be set to `1` if backoff is desired.

## Running experiments and generating graphs

After compilation, run `bash generate_all_graphs.sh`. Output graphs will be stored in the `graphs` directory, and timing results in `results`.
