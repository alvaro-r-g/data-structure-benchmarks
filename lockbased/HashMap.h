#pragma once

#include <memory>
#include <mutex>

#define TABLE_SIZE 10000

namespace lockbased {

class HashMap {
 private:
  struct Node {
    int key;
    int value;
    Node *next;
    Node *prev;
    Node() = default;
  };

  Node *bucket_heads[TABLE_SIZE];
  Node *bucket_tails[TABLE_SIZE];
  std::mutex hm_lock = {};

 public:
  HashMap() {
    for (int i = 0; i < TABLE_SIZE; i++) {
      bucket_heads[i] = new Node();
      bucket_tails[i] = new Node();
      bucket_heads[i]->next = bucket_tails[i];
      bucket_tails[i]->prev = bucket_heads[i];
    }
  }

  virtual ~HashMap() {
    for (int i = 0; i < TABLE_SIZE; i++) {

      Node *curr = bucket_heads[i]->next;
      Node *tail = bucket_tails[i];

      while (curr != tail) {
        Node *tmp = curr;
        curr = curr->next;
        delete tmp;
      }


      delete bucket_tails[i];
      delete bucket_heads[i];
    }
  }

  void insert_or_assign(int const& key, int const& value) {
    std::lock_guard<std::mutex> lock(hm_lock);
    unsigned long index = std::hash<int>{}(key) % TABLE_SIZE;
    
    Node *parent = bucket_heads[index];
    Node *curr = bucket_heads[index]->next;
    Node *tail = bucket_tails[index];

    while (curr != tail && curr->key != key) {
        parent = curr;
        curr = curr->next;
    }

    if (curr == tail) {
      Node* new_node = new Node();
      new_node->key = key;
      new_node->value = value;

      new_node->next = curr;
      new_node->prev = parent;
      
      parent->next = new_node;
      curr->prev = new_node;
    } else {
        curr->value = value;
    }
  }

  bool contains(int key) {
    std::lock_guard<std::mutex> lock(hm_lock);
    unsigned long index = std::hash<int>{}(key) % TABLE_SIZE;
    Node *curr = bucket_heads[index]->next;
    Node *tail = bucket_tails[index];

    while (curr != tail) {
      if (curr->key == key) {
        return true;
      } else {
        curr = curr->next;
      }
    }

    return false;
  }

  void remove(int const& key) {
    Node *tmp;
    {
      std::lock_guard<std::mutex> lock(hm_lock);
      unsigned long index = std::hash<int>{}(key) % TABLE_SIZE;

      Node *curr = bucket_heads[index]->next;
      Node *tail = bucket_tails[index];

      while (curr != tail && curr->key != key) {
        curr = curr->next;
      }

      if (curr == tail) return;

      tmp = curr;
      curr->next->prev = curr->prev;
      curr->prev->next = curr->next;
    }
    delete tmp;

    return;
  }

  int find(int key) {
    std::lock_guard<std::mutex> lock(hm_lock);
    unsigned long index = std::hash<int>{}(key) % TABLE_SIZE;

    Node *curr = bucket_heads[index]->next;
    Node *tail = bucket_tails[index];

     while (curr != tail) {
       if (curr->key == key) return curr->value;
       curr = curr->next;
    }

    return -1;
  }
  
};

}  // namespace lockbased
