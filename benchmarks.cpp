#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

#include "benchmarks.h"
#include "benchmark.h"
#include "configuration.h"
#include "tm.h"
#include "mcas.h"
#include "flock/flock.h"

#include "lockbased/Deque.h"
#include "lockbased/HashMap.h"
#include "lockbased/Queue.h"
#include "lockbased/SortedList.h"
#include "lockbased/Stack.h"
#include "lockbased/BinarySearchTree.h"
#include "lockbased/array_swap.h"

#include "lockfree/BinarySearchTree.h"
#include "lockfree/SortedList.h"
#include "lockfree/HashMap.h"
#include "lockfree/Deque.h"
#include "lockfree/Stack.h"
#include "lockfree/Queue.h"

#include "lockfree-mcas/BinarySearchTree.h"
#include "lockfree-mcas/Deque.h"
#include "lockfree-mcas/HashMap.h"
#include "lockfree-mcas/Queue.h"
#include "lockfree-mcas/SortedList.h"
#include "lockfree-mcas/Stack.h"
#include "lockfree-mcas/array_swap.h"

#include "lockfree-flock/array_swap.h"
#include "lockfree-flock/BinarySearchTree.h"
#include "lockfree-flock/HashMap.h"
// #include "lockfree-flock/LeafTree.h"
#include "lockfree-flock/SortedList.h"
#include "lockfree-flock/Deque.h"
#include "lockfree-flock/Stack.h"
#include "lockfree-flock/Queue.h"

#include "htm-lock/Deque.h"
#include "htm-lock/HashMap.h"
#include "htm-lock/Queue.h"
#include "htm-lock/SortedList.h"
#include "htm-lock/Stack.h"
#include "htm-lock/BinarySearchTree.h"
#include "htm-lock/array_swap.h"

#include "htm-mcas/Deque.h"
#include "htm-mcas/HashMap.h"
#include "htm-mcas/Queue.h"
#include "htm-mcas/SortedList.h"
#include "htm-mcas/Stack.h"
#include "htm-mcas/BinarySearchTree.h"

#include "flock-mcas/BinarySearchTree.h"
#include "flock-mcas/Deque.h"
#include "flock-mcas/Queue.h"
#include "flock-mcas/Stack.h"

static const int DATA_VALUE_RANGE_MIN = 0;
static const int DATA_VALUE_RANGE_MAX = 256;
static const int DATA_PREFILL = 1024;

void benchmark_mwobject(const Configuration& config) {
  struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
  } counters{};
  std::mutex counters_lock;

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    benchmark(config, u8"update", [&counters, &counters_lock](int random) {
      std::lock_guard<std::mutex> lock(counters_lock);
      counters.a++;
      counters.b++;
      counters.c++;
      counters.d++;
    });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

}

void benchmark_mcas_mwobject(const Configuration& config) {
  struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
  } counters{};

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    benchmark(config, u8"update", [&counters](int random) {
      while (true) {
        uint64_t old_a = counters.a;
        uint64_t old_b = counters.b;
        uint64_t old_c = counters.c;
        uint64_t old_d = counters.d;

        uint64_t new_a = old_a + 1;
        uint64_t new_b = old_b + 1;
        uint64_t new_c = old_c + 1;
        uint64_t new_d = old_d + 1;

        {
          if (qcas(reinterpret_cast<uint64_t*>(&counters.a), old_a, new_a,
                   reinterpret_cast<uint64_t*>(&counters.b), old_b, new_b,
                   reinterpret_cast<uint64_t*>(&counters.c), old_c, new_c,
                   reinterpret_cast<uint64_t*>(&counters.d), old_d, new_d))
            break;
        }
      }
    });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

}

void benchmark_htm_lock_mwobject(const Configuration& config) {
  struct {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
  } counters{};

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    benchmark(config, u8"update", [&counters] (int random) {
      TM_BEGIN(0);
      counters.a++;
      counters.b++;
      counters.c++;
      counters.d++;
      TM_END(0);
    });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

}

void benchmark_flock_mwobject(const Configuration& config) {
  struct target : flck::lock {
    flck::atomic<char *> a;
    flck::atomic<char *> b;
    flck::atomic<char *> c;
    flck::atomic<char *> d;
  };

  target counters;
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    benchmark(config, u8"update", [&counters](int random) {
      flck::with_epoch([&counters] {
        // flck lambdas should generally capture by value because their usage
        // might outlive local context, but here it means copying a pointer to
        // counters and counters will outlive usage of the lambda
        counters.with_lock([&counters] {
          counters.a = (counters.a).load() + 1;
          counters.b = (counters.b).load() + 1;
          counters.c = (counters.c).load() + 1;
          counters.d = (counters.d).load() + 1;
          return true;
        });
      });
    });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif
}

void benchmark_arrayswap(const Configuration& config) {
    lockbased::ArraySwap::initialize();

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    benchmark(config, u8"update", [](int random) {
      /* set up random number generator */
      std::random_device rd;
      std::mt19937 engine(rd());
      std::uniform_int_distribution<int> uniform_dist(0, lockbased::ArraySwap::NUM_ROWS-1);

      int index_a = uniform_dist(engine);
      int index_b = uniform_dist(engine);
      lockbased::ArraySwap::swap(index_a, index_b);
    });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

  lockbased::ArraySwap::datum_free(lockbased::ArraySwap::S);
}

void benchmark_mcas_arrayswap(const Configuration& config) {
  lockfree_mcas::ArraySwap::initialize();

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    benchmark(config, u8"update", [](int random) {
      /* set up random number generator */
      std::random_device rd;
      std::mt19937 engine(rd());
      std::uniform_int_distribution<int> uniform_dist(0, lockfree_mcas::ArraySwap::NUM_ROWS-1);

      int index_a = uniform_dist(engine);
      int index_b = uniform_dist(engine);
      lockfree_mcas::ArraySwap::swap(index_a, index_b);
    });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

  lockfree_mcas::ArraySwap::datum_free(lockfree_mcas::ArraySwap::S);
}

void benchmark_htm_lock_arrayswap(const Configuration& config) {
  htm_lock::ArraySwap::initialize();

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    benchmark(config, u8"update", [](int random) {
      /* set up random number generator */
      std::random_device rd;
      std::mt19937 engine(rd());
      std::uniform_int_distribution<int> uniform_dist(0, htm_lock::ArraySwap::NUM_ROWS-1);

      int index_a = uniform_dist(engine);
      int index_b = uniform_dist(engine);
      htm_lock::ArraySwap::swap(index_a, index_b);
    });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

  htm_lock::ArraySwap::datum_free(htm_lock::ArraySwap::S);
}

void benchmark_flock_arrayswap(const Configuration& config) {
  lockfree_flock::ArraySwap::initialize();

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
  benchmark(config, u8"update", [](int random) {
    /* set up random number generator */
    std::random_device rd;
    std::mt19937 engine(rd());
    std::uniform_int_distribution<int> uniform_dist(0, lockfree_flock::ArraySwap::NUM_ROWS-1);

    int index_a = uniform_dist(engine);
    int index_b = uniform_dist(engine);
    lockfree_flock::ArraySwap::swap(index_a, index_b);
  });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

  lockfree_flock::ArraySwap::datum_free(lockfree_flock::ArraySwap::S);
}

template <typename Deque>
void benchmark_deque(Deque& deque, const Configuration& config) {
  /* set up random number generator */
  std::random_device rd;
  std::mt19937 engine(rd());
  std::uniform_int_distribution<int> uniform_dist(DATA_VALUE_RANGE_MIN,
                                                  DATA_VALUE_RANGE_MAX);

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    // prefill deque with 1024 elements
    for (int i = 0; i < DATA_PREFILL; i++) {
      deque.push_back(uniform_dist(engine));
    }

    benchmark(config, u8"update", [&deque](int random) {
      auto choice1 =
          (random % (2 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
      auto choice2 =
          (random % (2 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
      if (choice1 == 0) {
        if (choice2 == 0) {
          deque.push_back(random % DATA_VALUE_RANGE_MAX);
        } else {
          deque.push_front(random % DATA_VALUE_RANGE_MAX);
        }
      } else {
        if (choice2 == 0) {
          deque.pop_back();
        } else {
          deque.pop_front();
        }
      }
    });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

}

template <typename Deque>
void benchmark_deque_lf(Deque& deque, const Configuration& config) {
  /* set up random number generator */
  std::random_device rd;
  std::mt19937 engine(rd());
  std::uniform_int_distribution<int> uniform_dist(DATA_VALUE_RANGE_MIN,
                                                  DATA_VALUE_RANGE_MAX);
  const std::thread::id MAIN_THREAD_ID = std::this_thread::get_id();

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    auto deque_worker = std::move(deque.first);
    auto deque_stealer = std::move(deque.second);
    // prefill deque with 1024 elements
    for (int i = 0; i < DATA_PREFILL; i++) {
      // deque.push_back(uniform_dist(engine));
      deque_worker.push(uniform_dist(engine));
    }

    benchmark(config, u8"update",
              [&deque, &deque_worker, &deque_stealer, MAIN_THREAD_ID](int random) {
      if (std::this_thread::get_id() == MAIN_THREAD_ID)
      {
        auto choice1 =
            (random % (2 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
        if (choice1 == 0) {
          deque_worker.push(random % DATA_VALUE_RANGE_MAX);
        } else {
          deque_worker.pop();
        }
      } else {
        auto clone = deque_stealer;
        clone.steal();
      }
    });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

}


template <typename Stack>
void benchmark_stack(Stack& stack, const Configuration& config) {
  /* set up random number generator */
  std::random_device rd;
  std::mt19937 engine(rd());
  std::uniform_int_distribution<int> uniform_dist(DATA_VALUE_RANGE_MIN,
                                                  DATA_VALUE_RANGE_MAX);

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    // prefill stack with 1024 elements
    for (int i = 0; i < DATA_PREFILL; i++) {
      stack.push(uniform_dist(engine));
    }

    benchmark(config, u8"update", [&stack](int random) {
      auto choice =
          (random % (2 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
      if (choice == 0) {
        stack.push(random % DATA_VALUE_RANGE_MAX);
      } else {
        stack.pop();
      }
    });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

}

template <typename Queue>
void benchmark_queue(Queue& queue, const Configuration& config) {
  /* set up random number generator */
  std::random_device rd;
  std::mt19937 engine(rd());
  std::uniform_int_distribution<int> uniform_dist(DATA_VALUE_RANGE_MIN,
                                                  DATA_VALUE_RANGE_MAX);

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    // prefill queue with 1024 elements
    for (int i = 0; i < DATA_PREFILL; i++) {
      queue.push(uniform_dist(engine));
    }

    benchmark(config, u8"update", [&queue](int random) {
      auto choice =
          (random % (2 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
      if (choice == 0) {
        queue.push(random % DATA_VALUE_RANGE_MAX);
      } else {
        queue.pop();
      }
    });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

}

template <typename List>
void read(List& l, int random) {
  /* read operations: 100% read */
  l.count(random % DATA_VALUE_RANGE_MAX);
}

template <typename List>
void update(List& l, int random) {
  /* update operations: 50% insert, 50% remove */
  auto choice = (random % (2 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
  if (choice == 0) {
    l.insert(random % DATA_VALUE_RANGE_MAX);
  } else {
    l.remove(random % DATA_VALUE_RANGE_MAX);
  }
}

template <typename List>
void mixed(List& l, int random) {
  /* mixed operations: 20% update, 80% read */
  auto choice = (random % (10 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
  if (choice == 0) {
    l.insert(random % DATA_VALUE_RANGE_MAX);
  } else if (choice == 1) {
    l.remove(random % DATA_VALUE_RANGE_MAX);
  } else {
    l.count(random % DATA_VALUE_RANGE_MAX);
  }
}

template <typename List>
void benchmark_sorted_list(List& list1, List& list2,
                           const Configuration& config) {
  /* set up random number generator */
  std::random_device rd;
  std::mt19937 engine(rd());
  std::uniform_int_distribution<int> uniform_dist(DATA_VALUE_RANGE_MIN,
                                                  DATA_VALUE_RANGE_MAX);

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {

    /* prefill list with 1024 elements */
    for (int i = 0; i < DATA_PREFILL; i++) {
      list1.insert(uniform_dist(engine));
    }
    benchmark(config, u8"read",
              [&list1](int random) { read(list1, random); });
    benchmark(config, u8"update",
              [&list1](int random) { update(list1, random); });
  }

  {
    /* prefill list with 1024 elements */
    for (int i = 0; i < DATA_PREFILL; i++) {
      list2.insert(uniform_dist(engine));
    }
    benchmark(config, u8"mixed", [&list2](int random) { mixed(list2, random); });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

}

template <typename List>
void sanity_check_sorted_list(List& list) {
  list.insert(1);
  list.insert(2);
  list.insert(3);
  list.insert(4);
  list.insert(4);
  list.insert(3);
  list.insert(2);
  list.insert(1);
  std::cout << "Should be 1, 1, 2, 2, 3, 3, 4, 4" << std::endl;
  list.print();
  std::cout << "count(3): " << list.count(3) << std::endl;
  list.remove(1);
  list.remove(2);
  list.remove(3);
  list.remove(4);
  list.remove(7);
  list.remove(0);
  list.remove(5);
  std::cout << "Should be 1, 2, 3, 4" << std::endl;
  list.print();  // 1, 2, 3, 4,
  std::cout << "count(5): " << list.count(5) << std::endl;
  list.remove(1);
  list.remove(1);
  std::cout << "Should be 2, 3, 4" << std::endl;
  list.print();  // 2, 3, 4,
  std::cout << "count(1): " << list.count(1) << std::endl;
  list.remove(2);
  list.remove(3);
  list.remove(4);
  std::cout << "Should be empty" << std::endl;
  list.print();  //
}

template <typename Map>
void sanity_check_hash_map(Map& map) {
  map.insert_or_assign(1, 0);
  map.insert_or_assign(3, 300);
  map.insert_or_assign(4, 7);
  map.insert_or_assign(4, 400);
  map.insert_or_assign(3, 300);
  map.insert_or_assign(2, 200);
  map.insert_or_assign(1, 100);
  
  std::cout << "Should be 3[300], 1[100], 4[400], 2[200]," << std::endl;
  map.print();
  map.remove(1);
  map.remove(2);
  map.remove(4);
  map.remove(7);
  map.remove(0);
  map.remove(5);
  std::cout << "Should be 3[300]," << std::endl;
  map.print();
  map.remove(3);
  std::cout << "Should be empty" << std::endl;
  map.print();  //
}

template <typename Tree>
void sanity_check_btree(Tree& tree) {
  std::cout << "Should be empty: ";
  tree.print();
  std::cout << "Min:" << tree.get_min() << std::endl;

  tree.insert(0);
  tree.insert(300);
  tree.insert(7);
  tree.insert(400);
  std::cout << "Should contain 0, 7, 300, 400: ";
  tree.print();
  std::cout << "Min:" << tree.get_min() << std::endl;
  
  tree.insert(300);
  tree.insert(200);
  tree.insert(100);
  std::cout << "Should contain 0, 7, 100, 200, 300, 300, 400: ";
  tree.print();
  std::cout << "Min:" << tree.get_min() << std::endl;

  tree.remove(4);
  tree.remove(7);
  tree.remove(0);
  tree.remove(5);
  tree.remove(300);
  std::cout << "Should contain 100, 200, 300, 400: ";
  tree.print();
  std::cout << "Min:" << tree.get_min() << std::endl;
}

template <typename Deque>
void sanity_check_deque(Deque& deque) {
  deque.push_front(0);
  deque.push_front(300);
  deque.push_front(7);
  deque.push_front(400);

  assert(deque.pop_front() == 400);
  assert(deque.pop_front() == 7);
  assert(deque.pop_front() == 300);
  assert(deque.pop_front() == 0);

  deque.push_back(300);
  deque.push_back(200);
  deque.push_back(100);

  assert(deque.pop_back() == 100);
  assert(deque.pop_front() == 300);
  assert(deque.pop_back() == 200);
}

template <typename HashMap>
void hm_lookup(HashMap& map, int random) {
  /* read operations: 100% read */
  map.contains(random % DATA_VALUE_RANGE_MAX);
}

template <typename HashMap>
void hm_update(HashMap& map, int random) {
  /* update operations: 50% insert, 50% remove */
  auto choice = (random % (2 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
  if (choice == 0) {
    map.insert_or_assign(random % DATA_VALUE_RANGE_MAX,
                         random % DATA_VALUE_RANGE_MAX);
  } else {
    map.remove(random % DATA_VALUE_RANGE_MAX);
  }
}

template <typename HashMap>
void hm_mixed(HashMap& map, int random) {
  /* mixed operations: 20% update, 80% read */
  auto choice = (random % (10 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
  if (choice == 0) {
    map.insert_or_assign(random % DATA_VALUE_RANGE_MAX,
                         random % DATA_VALUE_RANGE_MAX);
  } else if (choice == 1) {
    map.remove(random % DATA_VALUE_RANGE_MAX);
  } else {
    map.contains(random % DATA_VALUE_RANGE_MAX);
  }
}

template <typename HashMap>
void benchmark_hashmap(HashMap& map1, HashMap& map2,
                       const Configuration& config) {
  /* set up random number generator */
  std::random_device rd;
  std::mt19937 engine(rd());
  std::uniform_int_distribution<int> uniform_dist(DATA_VALUE_RANGE_MIN,
                                                  DATA_VALUE_RANGE_MAX);

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    /* prefill list with 1024 elements */
    for (int i = 0; i < DATA_PREFILL; i++) {
      map1.insert_or_assign(uniform_dist(engine), uniform_dist(engine));
    }
    benchmark(config, u8"read",
              [&map1](int random) { hm_lookup(map1, random); });
    benchmark(config, u8"update",
              [&map1](int random) { hm_update(map1, random); });
  }

  {
    /* prefill list with 1024 elements */
    for (int i = 0; i < DATA_PREFILL; i++) {
      map2.insert_or_assign(uniform_dist(engine), uniform_dist(engine));
    }
    benchmark(config, u8"mixed",
              [&map2](int random) { hm_mixed(map2, random); });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

}

template <typename BST>
void bst_lookup(BST& bst) {
  /* read operations: 100% read */
  bst.get_min();
}

template <typename BST>
void bst_update(BST& bst, int random) {
  /* update operations: 50% insert, 50% remove */
  auto choice = (random % (2 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
  if (choice == 0) {
    bst.insert(random % DATA_VALUE_RANGE_MAX);
  } else {
    bst.remove(random % DATA_VALUE_RANGE_MAX);
  }
}

template <typename BST>
void bst_mixed(BST& bst, int random) {
  /* mixed operations: 20% update, 80% read */
  auto choice = (random % (10 * DATA_VALUE_RANGE_MAX)) / DATA_VALUE_RANGE_MAX;
  if (choice == 0) {
    bst.insert(random % DATA_VALUE_RANGE_MAX);
  } else if (choice == 1) {
    bst.remove(random % DATA_VALUE_RANGE_MAX);
  } else {
    bst.get_min();
  }
}

template <typename BST>
void benchmark_bst(BST& bst1, BST& bst2, const Configuration& config) {
  /* set up random number generator */
  std::random_device rd;
  std::mt19937 engine(rd());
  std::uniform_int_distribution<int> uniform_dist(DATA_VALUE_RANGE_MIN,
                                                  DATA_VALUE_RANGE_MAX);

#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_begin();
#endif
  {
    /* prefill list with 1024 elements */
    for (int i = 0; i < DATA_PREFILL; i++) {
      bst1.insert(uniform_dist(engine));
    }
    benchmark(config, u8"read",
              [&bst1](int random) { bst_lookup(bst1); });
    benchmark(config, u8"update",
              [&bst1](int random) { bst_update(bst1, random); });
  }

  {
    /* prefill list with 1024 elements */
    for (int i = 0; i < DATA_PREFILL; i++) {
      bst2.insert(uniform_dist(engine));
    }
    benchmark(config, u8"mixed",
              [&bst2](int random) { bst_mixed(bst2, random); });
  }
#ifdef ENABLE_PARSEC_HOOKS
  __parsec_roi_end();
#endif

}

void run_benchmarks(const Configuration& config) {
  if (Configuration::is_htm(config.sync_type)) {
    TM_INIT();
  }
  switch (config.sync_type) {
    case Configuration::SyncType::LOCK: {
      switch (config.benchmarking_algorithm) {
        case Configuration::BenchmarkAlgorithm::MWOBJECT: {
          std::cout << "Benchmark Locking MWObject" << std::endl;
          benchmark_mwobject(config);
        } break;
        case Configuration::BenchmarkAlgorithm::ARRAYSWAP: {
          std::cout << "Benchmark Locking Array Swap" << std::endl;
          benchmark_arrayswap(config);
        } break;
        case Configuration::BenchmarkAlgorithm::STACK: {
          std::cout << "Benchmark Locking Stack" << std::endl;
          lockbased::Stack stack;
          benchmark_stack(stack, config);
        } break;
        case Configuration::BenchmarkAlgorithm::QUEUE: {
          std::cout << "Benchmark Locking Queue" << std::endl;
          lockbased::Queue queue;
          benchmark_queue(queue, config);
        } break;
        case Configuration::BenchmarkAlgorithm::DEQUE: {
          std::cout << "Benchmark Locking Deque" << std::endl;
          lockbased::Deque deque;
          benchmark_deque(deque, config);
        } break;
        case Configuration::BenchmarkAlgorithm::SORTEDLIST: {
          std::cout << "Benchmark Locking Sorted List" << std::endl;
          lockbased::SortedList list1;
          lockbased::SortedList list2;
          benchmark_sorted_list(list1, list2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::HASHMAP: {
          std::cout << "Benchmark Locking HashMap" << std::endl;
          lockbased::HashMap map1;
          lockbased::HashMap map2;
          benchmark_hashmap(map1, map2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::BST: {
          std::cout << "Benchmark Locking BST" << std::endl;
          lockbased::BinarySearchTree bst1;
          lockbased::BinarySearchTree bst2;
          benchmark_bst(bst1, bst2, config);
        } break;
        case Configuration::ALG_UNDEF: {
          std::cerr << "ALG_UNDEF" << std::endl;
        } break;
      }
    } break;
    case Configuration::SyncType::LOCKFREE: {
      switch (config.benchmarking_algorithm) {
        case Configuration::BenchmarkAlgorithm::MWOBJECT: {
          std::cerr << "MWOBJECT not implemented for lock-free" << std::endl;
        } break;
        case Configuration::BenchmarkAlgorithm::ARRAYSWAP: {
          std::cerr << "ARRAYSWAP not implemented for lock-free" << std::endl;
        } break;
        case Configuration::BenchmarkAlgorithm::STACK: {
          std::cout << "Benchmark Lock-Free Stack" << std::endl;
          lockfree::Stack<int> stack;
          benchmark_stack(stack, config);
        } break;
        case Configuration::BenchmarkAlgorithm::QUEUE: {
          std::cout << "Benchmark Lock-Free Queue" << std::endl;
          lockfree::Queue queue;
          benchmark_queue(queue, config);
        } break;
        case Configuration::BenchmarkAlgorithm::DEQUE: {
          std::cout << "Benchmark Lock-Free Deque" << std::endl;
          auto lf_spmc_deque = lockfree::deque::deque<int>();
          benchmark_deque_lf(lf_spmc_deque, config);
        } break;
        case Configuration::BenchmarkAlgorithm::SORTEDLIST: {
          std::cout << "Benchmark Lock-Free Sorted List" << std::endl;
          lockfree::SortedList list1;
          lockfree::SortedList list2;
          benchmark_sorted_list(list1, list2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::HASHMAP: {
          std::cout << "Benchmark Lock-Free HashMap" << std::endl;
          lockfree::HashMap map1;
          lockfree::HashMap map2;
          benchmark_hashmap(map1, map2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::BST: {
          std::cout << "Benchmark Lock-Free BST" << std::endl;
          lockfree::BinarySearchTree bst1;
          // lockfree::BinarySearchTree bst2;
          benchmark_bst(bst1, bst1, config);
        } break;
        case Configuration::ALG_UNDEF: {
          std::cerr << "ALG_UNDEF" << std::endl;
        } break;
      }
    } break;
    case Configuration::SyncType::LOCKFREE_MCAS:
    case Configuration::SyncType::MCAS_NO_IF:
    case Configuration::SyncType::MCAS_HTM:
    case Configuration::SyncType::MCAS_HTM_NO_IF:{
      std::string sync_name;
      switch (config.sync_type)
      {
        case Configuration::SyncType::LOCKFREE_MCAS:
          sync_name = "Lock-Free MCAS";
          break;
        case Configuration::SyncType::MCAS_NO_IF:
          sync_name = "MCAS no if";
          break;
        case Configuration::SyncType::MCAS_HTM:
          sync_name = "MCAS HTM";
          break;
        case Configuration::SyncType::MCAS_HTM_NO_IF:
          sync_name = "MCAS HTM no if";
          break;
        default:
          break;
      }
#ifdef MCAS_GEM5
      if (config.sync_type != Configuration::SyncType::LOCKFREE_MCAS) {
        std::cerr << "Error: trying to change MCAS emulation type but MCAS_GEM5 macro defined" << std::endl
                  << "Change sync_type to lockfree-mcas to use gem5 MCAS" << std::endl;
        break;
      }
#else
      set_mcas_type(config.sync_type);
#endif
      switch (config.benchmarking_algorithm) {
        case Configuration::BenchmarkAlgorithm::MWOBJECT: {
          std::cout << "Benchmark " << sync_name << " MWObject" << std::endl;
          benchmark_mcas_mwobject(config);
        } break;
        case Configuration::BenchmarkAlgorithm::ARRAYSWAP: {
          std::cout << "Benchmark " << sync_name << " Array Swap" << std::endl;
          benchmark_mcas_arrayswap(config);
        } break;
        case Configuration::BenchmarkAlgorithm::STACK: {
          std::cout << "Benchmark " << sync_name << " Stack" << std::endl;
          lockfree_mcas::Stack<int> stack;
          benchmark_stack(stack, config);
        } break;
        case Configuration::BenchmarkAlgorithm::QUEUE: {
          std::cout << "Benchmark " << sync_name << " Queue" << std::endl;
          lockfree_mcas::Queue<int> queue;
          benchmark_queue(queue, config);
        } break;
        case Configuration::BenchmarkAlgorithm::DEQUE: {
          std::cout << "Benchmark " << sync_name << " Deque" << std::endl;
          lockfree_mcas::Deque deque;
          benchmark_deque(deque, config);
        } break;
        case Configuration::BenchmarkAlgorithm::SORTEDLIST: {
          std::cout << "Benchmark " << sync_name << " Sorted List" << std::endl;
          lockfree_mcas::SortedList list1;
          lockfree_mcas::SortedList list2;
          benchmark_sorted_list(list1, list2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::HASHMAP: {
          std::cout << "Benchmark " << sync_name << " HashMap" << std::endl;
          lockfree_mcas::HashMap map1;
          lockfree_mcas::HashMap map2;
          benchmark_hashmap(map1, map2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::BST: {
          std::cout << "Benchmark " << sync_name << " BST" << std::endl;
          lockfree_mcas::BinarySearchTree bst1;
          lockfree_mcas::BinarySearchTree bst2;
          benchmark_bst(bst1, bst2, config);
        } break;
        case Configuration::ALG_UNDEF: {
          std::cerr << "ALG_UNDEF" << std::endl;
        } break;
      }
    } break;
    case Configuration::SyncType::LOCKFREE_FLOCK: {
      switch (config.benchmarking_algorithm) {
        case Configuration::BenchmarkAlgorithm::MWOBJECT: {
          std::cout << "Benchmark Lock-Free Flock MWObject" << std::endl;
          benchmark_flock_mwobject(config);
        } break;
        case Configuration::BenchmarkAlgorithm::ARRAYSWAP: {
          std::cout << "Benchmark Lock-Free Flock Array Swap" << std::endl;
          benchmark_flock_arrayswap(config);
        } break;
        case Configuration::BenchmarkAlgorithm::STACK: {
          std::cout << "Benchmark Lock-Free Flock Stack" << std::endl;
          lockfree_flock::Stack<int> stack;
          benchmark_stack(stack, config);
        } break;
        case Configuration::BenchmarkAlgorithm::QUEUE: {
          std::cout << "Benchmark Lock-Free Flock Queue" << std::endl;
          lockfree_flock::Queue<int> queue;
          benchmark_queue(queue, config);
        } break;
        case Configuration::BenchmarkAlgorithm::DEQUE: {
          std::cout << "Benchmark Lock-Free Flock Deque" << std::endl;
          lockfree_flock::Deque deque;
          benchmark_deque(deque, config);
        } break;
        case Configuration::BenchmarkAlgorithm::SORTEDLIST: {
          std::cout << "Benchmark Lock-Free Flock Sorted List" << std::endl;
          lockfree_flock::SortedList<int> list1;
          lockfree_flock::SortedList<int> list2;
          benchmark_sorted_list(list1, list2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::HASHMAP: {
          std::cout << "Benchmark Lock-Free Flock HashMap" << std::endl;
          lockfree_flock::HashMap<int, int> map1;
          lockfree_flock::HashMap<int, int> map2;
          benchmark_hashmap(map1, map2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::BST: {
          std::cout << "Benchmark Lock-Free Flock BST" << std::endl;
          lockfree_flock::BinarySearchTree bst1;
          lockfree_flock::BinarySearchTree bst2;
          benchmark_bst(bst1, bst2, config);
        } break;
        case Configuration::ALG_UNDEF: {
          std::cerr << "ALG_UNDEF" << std::endl;
        } break;
      }
    } break;
    case Configuration::SyncType::HTM_LOCK: {
      switch (config.benchmarking_algorithm) {
        case Configuration::BenchmarkAlgorithm::MWOBJECT: {
          std::cout << "Benchmark HTM Lock MWObject" << std::endl;
          benchmark_htm_lock_mwobject(config);
        } break;
        case Configuration::BenchmarkAlgorithm::ARRAYSWAP: {
          std::cout << "Benchmark HTM Lock Array Swap" << std::endl;
          benchmark_htm_lock_arrayswap(config);
        } break;
        case Configuration::BenchmarkAlgorithm::STACK: {
          std::cout << "Benchmark HTM Lock Stack" << std::endl;
          htm_lock::Stack stack;
          benchmark_stack(stack, config);
        } break;
        case Configuration::BenchmarkAlgorithm::QUEUE: {
          std::cout << "Benchmark HTM Lock Queue" << std::endl;
          htm_lock::Queue queue;
          benchmark_queue(queue, config);
        } break;
        case Configuration::BenchmarkAlgorithm::DEQUE: {
          std::cout << "Benchmark HTM Lock Deque" << std::endl;
          htm_lock::Deque deque;
          benchmark_deque(deque, config);
        } break;
        case Configuration::BenchmarkAlgorithm::SORTEDLIST: {
          std::cout << "Benchmark HTM Lock Sorted List" << std::endl;
          htm_lock::SortedList list1;
          htm_lock::SortedList list2;
          benchmark_sorted_list(list1, list2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::HASHMAP: {
          std::cout << "Benchmark HTM Lock HashMap" << std::endl;
          htm_lock::HashMap map1;
          htm_lock::HashMap map2;
          benchmark_hashmap(map1, map2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::BST: {
          std::cout << "Benchmark HTM Lock BST" << std::endl;
          htm_lock::BinarySearchTree bst1;
          htm_lock::BinarySearchTree bst2;
          benchmark_bst(bst1, bst2, config);
        } break;
        case Configuration::ALG_UNDEF: {
          std::cerr << "ALG_UNDEF" << std::endl;
        } break;
      }
    } break;
    case Configuration::SyncType::HTM_MCAS: {
      switch (config.benchmarking_algorithm) {
        case Configuration::BenchmarkAlgorithm::MWOBJECT: {
          std::cerr << "MWOBJECT not implemented for htm-mcas" << std::endl;
        } break;
        case Configuration::BenchmarkAlgorithm::ARRAYSWAP: {
          std::cerr << "ARRAYSWAP not implemented for htm-mcas" << std::endl;
        } break;
        case Configuration::BenchmarkAlgorithm::STACK: {
          std::cout << "Benchmark HTM MCAS Stack" << std::endl;
          htm_mcas::Stack<int> stack;
          benchmark_stack(stack, config);
        } break;
        case Configuration::BenchmarkAlgorithm::QUEUE: {
          std::cout << "Benchmark HTM MCAS Queue" << std::endl;
          htm_mcas::Queue<int> queue;
          benchmark_queue(queue, config);
        } break;
        case Configuration::BenchmarkAlgorithm::DEQUE: {
          std::cout << "Benchmark HTM MCAS Deque" << std::endl;
          htm_mcas::Deque deque;
          benchmark_deque(deque, config);
        } break;
        case Configuration::BenchmarkAlgorithm::SORTEDLIST: {
          std::cout << "Benchmark HTM MCAS Sorted List" << std::endl;
          htm_mcas::SortedList list1;
          htm_mcas::SortedList list2;
          benchmark_sorted_list(list1, list2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::HASHMAP: {
          std::cout << "Benchmark HTM MCAS HashMap" << std::endl;
          htm_mcas::HashMap map1;
          htm_mcas::HashMap map2;
          benchmark_hashmap(map1, map2, config);
        } break;
        case Configuration::BenchmarkAlgorithm::BST: {
          std::cout << "Benchmark HTM MCAS BST" << std::endl;
          htm_mcas::BinarySearchTree bst1;
          htm_mcas::BinarySearchTree bst2;
          benchmark_bst(bst1, bst2, config);
        } break;
        case Configuration::ALG_UNDEF: {
          std::cerr << "ALG_UNDEF" << std::endl;
        } break;
      }
    } break;
    case Configuration::SyncType::FLOCK_MCAS: {
      switch (config.benchmarking_algorithm) {
        case Configuration::BenchmarkAlgorithm::MWOBJECT: {
          std::cerr << "MWOBJECT not implemented for Flock MCAS" << std::endl;
        } break;
        case Configuration::BenchmarkAlgorithm::ARRAYSWAP: {
          std::cerr << "ARRAYSWAP not implemented for Flock MCAS" << std::endl;
        } break;
        case Configuration::BenchmarkAlgorithm::STACK: {
          std::cout << "Benchmark Flock MCAS Stack" << std::endl;
          flock_mcas::Stack<int> stack;
          benchmark_stack(stack, config);
        } break;
        case Configuration::BenchmarkAlgorithm::QUEUE: {
          std::cout << "Benchmark Flock MCAS Queue" << std::endl;
          flock_mcas::Queue<int> queue;
          benchmark_queue(queue, config);
        } break;
        case Configuration::BenchmarkAlgorithm::DEQUE: {
          std::cout << "Benchmark Flock MCAS Deque" << std::endl;
          flock_mcas::Deque deque;
          benchmark_deque(deque, config);
        } break;
        case Configuration::BenchmarkAlgorithm::SORTEDLIST: {
          std::cerr << "SORTEDLIST not implemented for Flock MCAS" << std::endl;
        } break;
        case Configuration::BenchmarkAlgorithm::HASHMAP: {
          std::cerr << "HASHMAP not implemented for Flock MCAS" << std::endl;
        } break;
        case Configuration::BenchmarkAlgorithm::BST: {
          std::cout << "Benchmark Flock MCAS BST" << std::endl;
          flock_mcas::BinarySearchTree bst1;
          flock_mcas::BinarySearchTree bst2;
          benchmark_bst(bst1, bst2, config);
        } break;
        case Configuration::ALG_UNDEF: {
          std::cerr << "ALG_UNDEF" << std::endl;
        } break;
      }
    } break;
    case Configuration::SYNC_UNDEF: {
      std::cerr << "SYNC_UNDEF" << std::endl;
    } break;
  }

}
