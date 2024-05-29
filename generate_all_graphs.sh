#!/bin/bash

threads='[1,2,4,8,16,32,64,72,128,144]'
extra_args='-tenp' # Add g if you only want to generate graphs (so benchmarks won't be re-run)
ops=1000000
bin=build/data_structure_benchmarks

# Uncomment if testing multiple binaries
# for bin in {"build/data_structure_benchmarks_without_backoff","build/data_structure_benchmarks_with_backoff"}; do
  python3 run_experiments.py -b "$bin" "$extra_args" mwobject "$threads" "$ops" all
  python3 run_experiments.py -b "$bin" "$extra_args" arrayswap "$threads" "$ops" all
  python3 run_experiments.py -b "$bin" "$extra_args" stack "$threads" "$ops" all
  python3 run_experiments.py -b "$bin" "$extra_args" queue "$threads" 100000 all # queue only 10^5 ops
  python3 run_experiments.py -b "$bin" "$extra_args" deque "$threads" "$ops" all
  python3 run_experiments.py -b "$bin" "$extra_args" sorted-list "$threads" "$ops" not-mcas # not-mcas for list and hashmap
  python3 run_experiments.py -b "$bin" "$extra_args" hashmap "$threads" "$ops" not-mcas
  python3 run_experiments.py -b "$bin" "$extra_args" bst "$threads" "$ops" all
# done
