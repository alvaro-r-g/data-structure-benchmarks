#pragma once

#include <climits>
#include "mcas.h"

#define TABLE_SIZE 10000

namespace lockfree_mcas {

class HashMap {
 private:
  struct Node {
    long key;
    long value;
    Node *next;
    Node *prev;
    Node() = default;
  };

  Node *bucket_heads[TABLE_SIZE];
  Node *bucket_tails[TABLE_SIZE];

  bool insert_before(Node *next, Node *node) {
    unsigned long index = std::hash<int>{}(node->key) % TABLE_SIZE;
    Node *head = bucket_heads[index];
    Node *tail = bucket_tails[index];

    if (next == head) {
      return insert_after(next, node);
    }

    if ((next->prev == nullptr) ||
        ((next->next == nullptr) && next != tail)) {
      return false;
    }

    Node *prev = next->prev;

    node->next = next;
    node->prev = prev;
    if (qcas((uint64_t *)&node->prev->next, (uint64_t)node->next, (uint64_t)node,
             (uint64_t *)&next->prev, (uint64_t)node->prev, (uint64_t)node,
             (uint64_t *)&node->next, (uint64_t)node->next, (uint64_t)next,
             (uint64_t *)&node->prev, (uint64_t)node->prev, (uint64_t)prev
    )
        ) {
      return true;
    } else {
      return false;
    }
  }

  bool insert_after(Node *prev, Node *node) {
    unsigned long index = std::hash<long>{}(node->key) % TABLE_SIZE;
    Node *head = bucket_heads[index];
    Node *tail = bucket_tails[index];

    if (prev == tail) {
      return insert_before(prev, node);
    }

    if ((prev->next == nullptr) ||
        ((prev->prev == nullptr) && prev != head)) {
      return false;
    }

    Node *next = prev->next;

    node->prev = prev;
    node->next = next;
    if (qcas((uint64_t *)&prev->next, (uint64_t)node->next, (uint64_t)node,
             (uint64_t *)&node->next->prev, (uint64_t)node->prev, (uint64_t)node,
             (uint64_t *)&node->next, (uint64_t)node->next, (uint64_t)next,
             (uint64_t *)&node->prev, (uint64_t)node->prev, (uint64_t)prev
    )
        ) {
      return true;
    } else {
      return false;
    }
  }

  bool delete_node(Node *node) {
    unsigned long index = std::hash<long>{}(node->key) % TABLE_SIZE;
    Node *head = bucket_heads[index];
    Node *tail = bucket_tails[index];

    if (node == head || node == tail) {
      return true;
    }
    Node *prev = node->prev;
    Node *next = node->next;
    Node *tmp = nullptr;

    if ((prev == nullptr) && (next == nullptr)) return false; // was already deleted

    if(qcas((uint64_t*)&prev->next, (uint64_t)node, (uint64_t)next,
            (uint64_t*)&next->prev, (uint64_t)node, (uint64_t)prev,
            (uint64_t*)&node->next, (uint64_t)next, (uint64_t)tmp,
            (uint64_t*)&node->prev, (uint64_t)prev, (uint64_t)tmp)
        ) {
      return true;
    } else {
      return false;
    }
  }

 public:
  HashMap() {
    for (int i = 0; i < TABLE_SIZE; i++) {
      bucket_heads[i] = new Node();
      bucket_tails[i] = new Node();
      bucket_heads[i]->next = bucket_tails[i];
      bucket_heads[i]->value = LONG_MIN;
      bucket_tails[i]->prev = bucket_heads[i];
      bucket_tails[i]->value = LONG_MAX;
    }
  }

  void insert_or_assign(long key, long value) {
    unsigned long index = std::hash<long>{}(key) % TABLE_SIZE;
    Node *new_node = new Node();
    new_node->key = key;
    new_node->value = value;

    while (true) {
      retry:
      new_node->next = nullptr;
      new_node->prev = nullptr;

      Node *curr = bucket_heads[index]->next;
      Node *tail = bucket_tails[index];

      while (curr != nullptr && curr != tail && curr->key != key) {
        curr = curr->next;
      }

      if (curr == nullptr) goto retry;
      if (curr == tail) {
        if (insert_before(tail, new_node)) {
          return;
        } else {
          goto retry;
        }
      }
      if (curr->key == key) {
        if ((cas((uint64_t *)&curr->value, (uint64_t)curr->value, (uint64_t)value))) {
          delete new_node;
          return;
        } else {
          std::cout << "cas failed, retry\n";
          goto retry;
        }
      }
    }
  }

  bool contains(long key) {
    unsigned long index = std::hash<long>{}(key) % TABLE_SIZE;
    Node *curr = bucket_heads[index]->next;
    Node *tail = bucket_tails[index];

    while(true) {
      while (curr != tail && curr != nullptr) {
        if (curr->key == key) return true;
        curr = curr->next;
      }
      if (curr == tail) return false;
    }
  }

  void remove(long key) {
    unsigned long index = std::hash<long>{}(key) % TABLE_SIZE;

    while (true) {
      retry:
      Node *curr = bucket_heads[index]->next;
      Node *tail = bucket_tails[index];

      while (curr != nullptr && curr != tail && curr->key != key) {
        curr = curr->next;
      }

      if (curr == nullptr) goto retry;
      if (curr == tail) return;
      if (curr->key == key) {
        if (delete_node(curr)) return;
      }
    }
  }

  long find(long key) {
    unsigned long index = std::hash<long>{}(key) % TABLE_SIZE;

    Node *curr = bucket_heads[index]->next;
    Node *tail = bucket_tails[index];

    while(true) {
      while (curr != tail && curr != nullptr) {
        if (curr->key == key) return curr->value;
        curr = curr->next;
      }
      if (curr == tail) return LONG_MIN;
    }
  }
};

}  // namespace lockfree_mcas
