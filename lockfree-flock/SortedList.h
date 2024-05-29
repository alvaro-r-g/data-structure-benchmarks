// From https://github.com/cmuparlay/flock/blob/main/structures/dlist/set.h

#pragma once

#include "flock/flock.h"

namespace lockfree_flock {
template <typename K>
class SortedList {
  struct alignas(64) node : flck::lock {
    bool is_end;
    flck::atomic_write_once<bool> removed;
    flck::atomic<node*> prev;
    flck::atomic<node*> next;
    K key;
    node(K key, node* next, node* prev)
      : is_end(false), removed(false), prev(prev), next(next), key(key){};
    node(node* next)
      : is_end(true), removed(false), prev(nullptr), next(next) {}
  };

  flck::memory_pool<node> node_pool;
  node* root;

  auto find_location(K k) {
    node* nxt = (root->next).load();
    while (true) {
      node* nxt_nxt = (nxt->next).load(); // prefetch
      if (nxt->is_end || nxt->key >= k) break;
      nxt = nxt_nxt;
    }
    return nxt;
  }

  static constexpr int init_delay = 200;
  static constexpr int max_delay = 2000;

  node* empty() {
    node* tail = node_pool.new_obj(nullptr);
    node* head = node_pool.new_obj(tail);
    tail->prev = head;
    return head;
  }

  void retire(node* p, bool is_start = true) {
    if (is_start || !p->is_end) retire(p->next.load(), false);
    node_pool.retire(p);
  }

 public:
  SortedList() { root = empty(); }

  ~SortedList() { retire(root); }

  void insert(K k) {
    flck::with_epoch([&] {
      int delay = init_delay;
      while (true) {
        node* next = find_location(k);
        node* prev = (next->prev).load();
        // fails if something inserted before in meantime
        if ((prev->is_end || prev->key < k) && prev->try_lock([=] {
              if (!prev->removed.load() && (prev->next).load() == next) {
                auto new_node = node_pool.new_obj(k, next, prev);
                prev->next = new_node;
                next->prev = new_node;
                return true;
              } else
                return false;
            }))
          return true;
        for (volatile int i = 0; i < delay; i++)
          ;
        delay = std::min(2 * delay, max_delay);
      }
    });
  }

  void remove(K k) {
    flck::with_epoch([&] {
      int delay = init_delay;
      while (true) {
        node* loc = find_location(k);
        if (loc->is_end || loc->key != k) return;
        node* prev = (loc->prev).load();
        if (prev->try_lock([=] {
              if (prev->removed.load() || (prev->next).load() != loc)
                return false;
              return loc->try_lock([=] {
                node* next = (loc->next).load();
                loc->removed = true;
                prev->next = next;
                next->prev = prev;
                node_pool.retire(loc);
                return true;
              });
            }))
          return;
        for (volatile int i = 0; i < delay; i++)
          ;
        delay = std::min(2 * delay, max_delay);
      }
    });
  }

  int count(K data) {
    return flck::with_epoch([&]() -> int {
      int total = 0;
      auto cur = find_location(data);

      while (!cur->is_end && cur->key == data) {
        total++;
        cur = cur->next.load();
      }

      return total;
    });
  }

  void print() {
    node* ptr = (root->next).load();
    while (!ptr->is_end) {
      std::cout << ptr->key << ", ";
      ptr = (ptr->next).load();
    }
    std::cout << std::endl;
  }
};
}  // namespace lockfree_flock
