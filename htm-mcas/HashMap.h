// Adapted from lockfree-mcas/HashMap.h

#pragma once

#include <climits>
#include "tm.h"

#define TABLE_SIZE 10000

namespace htm_mcas {

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

  void insert_before(Node *next, Node *node) {
    unsigned long index = std::hash<int>{}(node->key) % TABLE_SIZE;
    Node *head = bucket_heads[index];

    if (next == head) {
      insert_after(next, node);
    }

    Node *prev = next->prev;

    node->next = next;
    node->prev = prev;

    node->prev->next = node;
    next->prev = node;
    // node->next = next;
    // node->prev = prev;
  }

  void insert_after(Node *prev, Node *node) {
    unsigned long index = std::hash<long>{}(node->key) % TABLE_SIZE;
    Node *tail = bucket_tails[index];

    if (prev == tail) {
      insert_before(prev, node);
    }

    Node *next = prev->next;

    node->prev = prev;
    node->next = next;

    prev->next = node;
    node->next->prev = node;
    // node->next = next;
    // node->prev = prev;
  }

  void delete_node(Node *node) {
    unsigned long index = std::hash<long>{}(node->key) % TABLE_SIZE;
    Node *head = bucket_heads[index];
    Node *tail = bucket_tails[index];

    if (node == head || node == tail) {
      return;
    }

    Node *prev = node->prev;
    Node *next = node->next;

    prev->next = next;
    next->prev = prev;
    // node->next = nullptr;
    // node->prev = nullptr;
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
    new_node->next = nullptr;
    new_node->prev = nullptr;

    TM_BEGIN(0);

    Node *curr = bucket_heads[index]->next;
    Node *tail = bucket_tails[index];

    while (curr != nullptr && curr != tail && curr->key != key) {
      curr = curr->next;
    }

    if (curr == tail) {
      insert_before(tail, new_node);
    } else if (curr->key == key) {
      curr->value = value;
      delete new_node;
    }

    TM_END(0);
  }

  bool contains(long key) {
    unsigned long index = std::hash<long>{}(key) % TABLE_SIZE;
    Node *curr = bucket_heads[index]->next;
    Node *tail = bucket_tails[index];

    TM_BEGIN(1);

    while (curr != tail) {
      if (curr->key == key) {
        TM_END(1);
        return true;
      }
      curr = curr->next;
    }

    TM_END(1);

    return false;
  }

  void remove(long key) {
    unsigned long index = std::hash<long>{}(key) % TABLE_SIZE;
    Node *curr = bucket_heads[index]->next;
    Node *tail = bucket_tails[index];

    TM_BEGIN(2);

    while (curr != nullptr && curr != tail && curr->key != key) {
      curr = curr->next;
    }

    if (curr->key == key) {
      delete_node(curr);
    }

    TM_END(2);
  }

  // long find(long key) {
  //   unsigned long index = std::hash<long>{}(key) % TABLE_SIZE;
  //
  //   Node *curr = bucket_heads[index]->next;
  //   Node *tail = bucket_tails[index];
  //
  //   while(true) {
  //     while (curr != tail && curr != nullptr) {
  //       if (curr->key == key) return curr->value;
  //       curr = curr->next;
  //     }
  //     if (curr == tail) return LONG_MIN;
  //   }
  // }
};

}  // namespace htm_mcas
