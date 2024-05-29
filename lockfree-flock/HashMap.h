// From https://github.com/cmuparlay/flock/blob/main/structures/hash/set.h

#pragma once

#include "flock/flock.h"
#include "parlay/primitives.h"

#define TABLE_SIZE 10000

namespace lockfree_flock {
template <typename K, typename V>
class HashMap {
  struct alignas(32) node {
    K key;
    V value;
    flck::atomic<node*> next;
    node(K key, V value, node* next) : key(key), value(value), next(next){};
  };

  struct slot : flck::lock {
    flck::atomic<node*> head;
    flck::atomic<unsigned int> version_num;
    slot() : head(nullptr), version_num(0) {}
  };

  using Table = parlay::sequence<slot>;

  flck::memory_pool<node> node_pool;
  Table table;

  slot* get_slot(K k) {
    return &table[(k * 0x9ddfea08eb382d69ULL) & (table.size() - 1u)];
  }

  auto find_in_slot(slot* s, K k) {
    auto cur = &s->head;
    node* nxt = cur->read();
    while (nxt != nullptr && nxt->key != k) {
      cur = &(nxt->next);
      nxt = cur->read();
    }
    return std::make_pair(cur, nxt);
  }

  std::optional<V> find(K k) {
    slot* s = get_slot(k);
    __builtin_prefetch(s);
    return flck::with_epoch([&]() -> std::optional<V> {
      auto [cur, nxt] = find_in_slot(s, k);
      cur->validate();
      if (nxt != nullptr)
        return nxt->value;
      else
        return {};
    });
  }

  std::optional<V> find_(K k) {
    slot* s = get_slot(table, k);
    auto [cur, nxt] = find_in_slot(s, k);
    cur->validate();
    if (nxt != nullptr)
      return nxt->value;
    else
      return {};
  }

  void insert_at(slot* s, K k, V v) {
    while (true) {
      unsigned int vn = s->version_num.load();
      auto [cur, nxt] = find_in_slot(s, k);
      if (s->try_lock([=] {
            if (s->version_num.load() != vn) return false;
            if (nxt != nullptr) {  // assign
              nxt->value = v;
              // s->version_num = vn + 1;
            } else {  // insert
              *cur = node_pool.new_obj(k, v, nullptr);
              s->version_num = vn + 1;
            }
            return true;
          }))
        break;
    }
  }

  bool remove_at(slot* s, K k) {
    while (true) {
      unsigned int vn = s->version_num.load();
      auto [cur, nxt] = find_in_slot(s, k);
      if (nxt == nullptr) return false;
      if (s->try_lock([=] {
            if (s->version_num.load() != vn) return false;
            *cur = nxt->next.load();
            node_pool.retire(nxt);
            s->version_num = vn + 1;
            return true;
          }))
        return true;
    }
  }

  Table empty(size_t n) {
    size_t size = (1ul << parlay::log2_up(n));
    return parlay::sequence<slot>(2 * size);
  }

  void retire_list(node* ptr) {
    if (ptr == nullptr)
      return;
    else {
      retire_list((ptr->next).load());
      node_pool.retire(ptr);
    }
  }

  void retire() {
    parlay::parallel_for(0, table.size(),
                         [&](size_t i) { retire_list(table[i].head.load()); });
    table.clear();
  }

  long check() {
    auto s = parlay::tabulate(table.size(), [&](size_t i) {
      node* ptr = table[i].head.load();
      int cnt = 0;
      while (ptr != nullptr) {
        cnt++;
        ptr = (ptr->next).load();
      }
      return cnt;
    });
    return parlay::reduce(s);
  }

  void clear() { node_pool.clear(); }
  void reserve(size_t n) { node_pool.reserve(n); }
  void shuffle(size_t n) { node_pool.shuffle(n); }
  void stats() { node_pool.stats(); }

 public:
  HashMap() { table = empty(TABLE_SIZE); }

  ~HashMap() { retire(); }

  void insert_or_assign(K k, V v) {
    slot* s = get_slot(k);
    flck::with_epoch([&] { insert_at(s, k, v); });
  }

  void remove(K k) {
    slot* s = get_slot(k);
    flck::with_epoch([&] { remove_at(s, k); });
  }

  bool contains(K k) { return find(k).has_value(); }

  void print() {
    for (size_t i = 0; i < table.size(); i++) {
      auto x = &(table[i].head);
      auto ptr = x->load();
      if (ptr != nullptr) {
        printf("\nFound used bucket %ld:", i);
      }
      while (ptr != nullptr) {
        std::cout << ptr->key << "[" << ptr->value << "], ";
        ptr = (ptr->next).load();
      }
    }
    std::cout << std::endl;
  }
};
}  // namespace lockfree_flock
