#pragma once

#include "DPointer.h"
#include "DoublyLinkedList.h"
#include <climits>

#define TABLE_SIZE 10000

namespace lockfree {

class Lockprogram {
 public:
  class LockFreehash {
   public:
    class Node {
     public:
      int item;
      int key;  // item’s hash code
      DPointer<LockFreehash::Node, sizeof(size_t)> next;
      Node() {}
      Node(int item1) {  // usual constructor
        this->key = item1;
        this->next = DPointer<LockFreehash::Node, sizeof(size_t)>();
      }
      Node(int key, int x) {
        this->key = key;
        this->item = x;
        this->next = DPointer<LockFreehash::Node, sizeof(size_t)>();
      }
      Node* getnext() {
        bool cMarked[] = {false};
        bool sMarked[] = {false};
        Node* succ;
        Node* entry = this->next.ptr;
        cMarked[0] = this->next.mark;
        while (cMarked[0]) {
          succ = entry->next.ptr;
          sMarked[0] = entry->next.mark;
          this->next.ptr = succ;
          this->next.mark = sMarked[0];
          entry = this->next.ptr;
          cMarked[0] = this->next.mark;
        }
        return entry;
      }
    };

   public:
    class Window {
     public:
      Node* pred;
      Node* curr;
      Window() {}
      Window(Node* pred, Node* curr) {
        this->pred = pred;
        this->curr = curr;
      }
    };
    const static int WORD_SIZE = 24;
    const static int LO_MASK = 0x00000001;
    const static int HI_MASK = 0x00800000;
    const static int MASK = 0x00FFFFFF;
    Node* head;
    LockFreehash() {
      this->head = new Node(0);
      Node* tail = new Node(2147483647);
      while (!head->next.cas(
          DPointer<LockFreehash::Node, sizeof(size_t)>(tail, 0), NULL))
        ;
    }
    LockFreehash(Node* e) { this->head = e; }
    int hashcode(int x) {
      std::hash<int> hash_fn;
      int a = hash_fn(x);
      return a & MASK;
    }
    int reverse(int key) {
      int loMask = LO_MASK;
      int hiMask = HI_MASK;
      int result = 0;
      for (int i = 0; i < WORD_SIZE; i++) {
        if ((key & loMask) != 0) {  // bit set
          result |= hiMask;
        }
        loMask <<= 1;
        hiMask >>= 1;  // fill with 0 from left
      }
      return result;
    }
    int makeRegularKey(int x) {
      std::hash<int> hash_fn;
      int a = hash_fn(x);
      int code = a & MASK;  // take 3 lowest bytes
      return reverse(code | HI_MASK);
    }
    int makeSentinelKey(int key) { return reverse(key & MASK); }
    Window* find(Node* head, int key) {
      Node* pred = head;
      Node* curr = head->getnext();
      while (curr->key < key) {
        pred = curr;
        curr = pred->getnext();
      }
      return new Window(pred, curr);
    }
    bool add(int x) {
      int key = makeRegularKey(x);
      bool splice;
      while (true) {
        Window* window = find(head, key);
        Node* pred = window->pred;
        Node* curr = window->curr;
        Node* entry = new Node(key, x);
        entry->next = DPointer<LockFreehash::Node, sizeof(size_t)>(curr, 0);
        splice = pred->next.cas(DPointer<Node, sizeof(size_t)>(entry, 0), curr);
        if (splice)
          return true;
        else
          continue;
      }
    }
    bool remove(int x) {
      int key = makeRegularKey(x);
      bool snip;
      while (true) {
        Window* window = find(head, key);
        Node* pred = window->pred;
        Node* curr = window->curr;
        if (curr->key != key) {
          return false;
        } else {
          snip =
              pred->next.cas(DPointer<Node, sizeof(size_t)>(curr, true), curr);
          if (snip)
            return true;
          else
            continue;
        }
      }
    }
    bool contains(int x) {
      int key = makeRegularKey(x);
      Window* window = find(head, key);
      Node* curr = window->curr;
      return curr->key == key;
    }
    LockFreehash* getsentinel(int index) {
      int key = makeSentinelKey(index);
      bool splice;
      while (true) {
        Window* window = find(head, key);
        Node* pred = window->pred;
        Node* curr = window->curr;
        // is the key present?
        if (curr->key == key) {
          return new LockFreehash(curr);
        } else {
          // splice in new entry
          Node* entry = new Node(key);
          entry->next = DPointer<LockFreehash::Node, sizeof(size_t)>(pred, 0);
          splice = pred->next.cas(DPointer<Node, sizeof(size_t)>(entry, false),
                                  curr);
          if (splice) {
            return new LockFreehash(curr);
          } else
            continue;
        }
      }
    }
  };
  std::vector<LockFreehash*> bucket;
  int bucketSize;
  int setSize;
  constexpr const static double THRESHOLD = 4.0;
  LockFreehash lfh;
  Lockprogram() {
    int capacity = TABLE_SIZE;
    for (int i = 0; i < capacity; i++) {
      LockFreehash* temp = new LockFreehash();
      this->bucket.push_back(temp);
    }
    this->bucketSize = 2;
    this->setSize = 0;
  }
  Lockprogram(int capacity) {
    for (int i = 0; i < capacity; i++) {
      LockFreehash* temp = new LockFreehash();
      this->bucket.push_back(temp);
    }
    this->bucketSize = 2;
    this->setSize = 0;
  }
  LockFreehash* getBucketList(int myBucket) {
    if (this->bucket[myBucket] == NULL) initializeBucket(myBucket);
    return this->bucket[myBucket];
  }
  int getparent(int myBucket) {
    int parent = this->bucketSize;
    do {
      parent = parent >> 1;
    } while (parent > myBucket);
    parent = myBucket - parent;
    return parent;
  }
  void initializeBucket(int myBucket) {
    int parent = getparent(myBucket);
    if (this->bucket[parent] == NULL) initializeBucket(parent);
    LockFreehash* b = this->bucket[parent]->getsentinel(myBucket);
    if (b != NULL) this->bucket[myBucket] = b;
  }
  bool add(int x) {
    int mybucket = abs(lfh.hashcode(x) % this->bucketSize);
    LockFreehash* b = getBucketList(mybucket);
    if (!b->add(x)) return false;
    int setSizeNow = this->setSize + 1;
    int bucketSizeNow = this->bucketSize;
    if (setSizeNow / (double)bucketSizeNow > THRESHOLD)
      this->bucketSize = 2 * bucketSizeNow;
    return true;
  }
  bool remove(int x) {
    int myBucket = abs(lfh.hashcode(x) % bucketSize);
    LockFreehash* b = getBucketList(myBucket);
    if (!b->remove(x)) return false;
    return true;
  }
  bool contains(int x) {
    int myBucket = abs(lfh.hashcode(x) % bucketSize);
    LockFreehash* b = getBucketList(myBucket);
    return b->contains(x);
  }
};

class HashMap {
 public:
  Lockprogram hm;

  HashMap() {}

  void insert_or_assign(int key, int value) {
    hm.add(key);
  }

  bool contains(int key) {
    return hm.contains(key);
  }

  void remove(int key) {
    hm.remove(key);
  }

  int find(int key) {
    return hm.contains(key);
  }

};

}  // namespace lockfree
