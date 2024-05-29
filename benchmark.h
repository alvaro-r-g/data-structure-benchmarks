/* a small benchmarking framework by David Klaftenegger, 2015
 * please report bugs or suggest improvements to david.klaftenegger@it.uu.se
 */


#pragma once

#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include <vector>
#include <iostream>
#include "tm.h"

enum class worker_status {wait, work, finish};

static const int RANDOM_VALUE_RANGE_MIN = 0;
static const int RANDOM_VALUE_RANGE_MAX = 65536;

pthread_barrier_t barrier;

/* template is used to allow functions/functors of any signature */
template<typename Function>
void worker(unsigned int random_seed, unsigned int n_ops, bool htm, Function fun) {
  /* set up random number generator */
  std::mt19937 engine(random_seed);
  std::uniform_int_distribution<int> uniform_dist(RANDOM_VALUE_RANGE_MIN, RANDOM_VALUE_RANGE_MAX);

  if (htm) {
    TM_THREAD_ENTER();
    // std::cout << "HTM thread " << random_seed << " entered" << endl;
  }

  pthread_barrier_wait(&barrier);

  for (unsigned int i = 0; i < n_ops; i++) {
    auto random = uniform_dist(engine);
    /* do specified work */
    fun(random);
  }
  
  pthread_barrier_wait(&barrier);

  if (htm) {
    TM_THREAD_EXIT();
  }
}


template<typename Function>
void benchmark(Configuration config, const std::string& identifier, Function fun) {
  unsigned int threadcnt = config.n_threads;
  unsigned int n_ops = config.n_ops;
  auto n_ops_per_thread = n_ops / threadcnt;
  bool htm = Configuration::is_htm(config.sync_type);
  /* spawn workers */
  std::vector<std::thread*> workers;
  std::random_device rd;

  pthread_barrier_init(&barrier, NULL, threadcnt);

  using clock = std::chrono::high_resolution_clock;

  for(unsigned int i = 0; i < threadcnt-1; i++) {
    auto seed = rd();
    auto w = new std::thread([seed, n_ops_per_thread, htm, fun]() { worker(seed, n_ops_per_thread, htm, fun); });
    workers.push_back(w);
    // set thread affinity for workers
    // cpu_set_t cpuset;
    // CPU_ZERO(&cpuset);
    // CPU_SET(i+1, &cpuset);
    // pthread_setaffinity_np(workers[i]->native_handle(), sizeof(cpu_set_t), &cpuset);
  };

  // set thread affinity for main thread
  // cpu_set_t cpuset;
  // CPU_ZERO(&cpuset);
  // CPU_SET(0, &cpuset);
  // pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);

  std::chrono::time_point<clock> start_time = clock::now();
  worker(rd(), n_ops_per_thread, htm, fun);
  std::chrono::time_point<clock> end_time = clock::now();

  /* make sure all workers terminated */
  for(auto& w : workers) {
    w->join();
    delete w;
  }

  workers.clear();
  pthread_barrier_destroy(&barrier);

  unsigned long long time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
  std::cout << u8"type: " << identifier << std::endl
            << u8"\tthreads: " << threadcnt << std::endl
            << u8"\tops: " << n_ops << std::endl
            << u8"\ttime(us): " << time << std::endl
  << u8"operation end" << std::endl;
}
