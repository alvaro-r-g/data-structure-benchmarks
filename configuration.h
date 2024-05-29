#pragma once

class Configuration{
public:
  enum SyncType{
    SYNC_UNDEF,
    LOCK,
    LOCKFREE,
    LOCKFREE_MCAS,
    LOCKFREE_FLOCK,
    HTM_LOCK,
    HTM_MCAS,
    MCAS_NO_IF,
    MCAS_HTM,
    MCAS_HTM_NO_IF,
    FLOCK_MCAS,
  };
  
  static bool is_htm(SyncType type) {
    return type == Configuration::HTM_LOCK ||
           type == Configuration::HTM_MCAS ||
           type == Configuration::MCAS_HTM ||
           type == Configuration::MCAS_HTM_NO_IF;
  }

  enum BenchmarkAlgorithm{
    ALG_UNDEF,
    MWOBJECT,
    ARRAYSWAP,
    STACK,
    QUEUE,
    DEQUE,
    SORTEDLIST,
    HASHMAP,
    BST,
  };

  Configuration(){
    n_threads = 1;
    sync_type = SYNC_UNDEF;
    benchmarking_algorithm = ALG_UNDEF;
    n_iter = 1;
    n_ops = 100;
    debug = false;
  };

  SyncType sync_type;
  BenchmarkAlgorithm benchmarking_algorithm;
  unsigned int n_threads;
  unsigned int n_iter;
  unsigned int n_ops;
  bool debug;
  static const Configuration default_conf;
};
