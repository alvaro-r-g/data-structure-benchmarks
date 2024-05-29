// From lockfree-mcas/BinarySearchTree.h

#pragma once

#include <cassert>
#include <climits>
#include <iostream>

#include "flock/flock.h"

namespace flock_mcas {

class BinarySearchTree {
 private:
  struct Node : flck::lock {
    flck::atomic<int> value;
    flck::atomic<Node*> left;
    flck::atomic<Node*> right;
    Node(int value) : value(value), left(nullptr), right(nullptr) {}
  };

  flck::memory_pool<Node> node_pool;

  flck::atomic<Node*> root;
  flck::lock root_lock;

  int sentinel_min = INT_MIN;
  int sentinel_max = INT_MAX;

  typedef enum {
    LEFT,
    RIGHT,
  } node_type;

 public:
  BinarySearchTree() : root(nullptr) {};

  void insert(const int value) {
    flck::with_epoch([=] {
      Node *new_node = node_pool.new_obj(value);

      if (root.load() == nullptr && root_lock.with_lock([=] {
        if (root.load() == nullptr) {
          root = new_node;
          return true;
        }
        return false;
      }))
        return;
      
      while (true) {
        Node *curr = root.load();
        Node *prev = nullptr;
        node_type type = LEFT;

        while (curr) {
          prev = curr;
          if (value < (curr->value).load()) {
            curr = (curr->left).load();
            type = LEFT;
          } else {
            curr = (curr->right).load();
            type = RIGHT;
          }
        }

        if (type == LEFT) {
          Node *last = (prev->left).load();

          if (prev->try_lock([=] {
            if ((prev->left).load() == last) {
              prev->left = new_node;
              return true;
            }
            return false;
          }))
            return;
        } else {
          Node *last = (prev->right).load();

          if (prev->try_lock([=] {
            if ((prev->right).load() == last) {
              prev->right = new_node;
              return true;
            }
            return false;
          }))
            return;
        }
      }
    });
  }

  void remove(int val) {
    flck::with_epoch([=] {
      int value = val;

    retry:
      Node *curr = root.load();
      Node *prev = nullptr;
      node_type type = LEFT;

      while (curr) {
        if ((curr->value).load() == value) {
          if (!(curr->left).load() && !(curr->right).load()) {                    // node to be removed has no children’s
            if (curr != root.load() && prev) { // delete leaf node
              if (type == LEFT) {
                if (prev->try_lock([=] {
                  if ((prev->left).load() != curr)
                    return false;
                  prev->left = nullptr;
                  node_pool.retire(curr);
                  return true;
                }))
                  return;
                goto retry;
              } else {
                if (prev->try_lock([=] {
                  if ((prev->right).load() != curr)
                    return false;
                  prev->right = nullptr;
                  node_pool.retire(curr);
                  return true;
                }))
                  return;
                goto retry;
              }
            } else {
              Node *last = root.load();

              if (root_lock.try_lock([=] {
                if (root.load() != last)
                  return false;
                root = nullptr;
                node_pool.retire(curr);
                return true;
              }))
                return;
            }  // deleted node is root
          } else if ((curr->left).load() && (curr->right).load()) {  // node to be removed has two children’s
            curr->value = get_min((curr->right).load());  // find minimum value from right subtree
            value = (curr->value).load();
            prev = curr;
            curr = (curr->right).load();  // continue from right subtree delete min node
            type = RIGHT;
            continue;
          } else {              // node to be removed has one children
            Node *rt = root.load();
            if (curr == rt) {
              Node *rl = (rt->left).load();
              if (rl) {
                if (root_lock.try_lock([=] {
                  if (root.load() != rt)
                    return false;
                  root = rl;
                  node_pool.retire(curr);
                  return true;
                }))
                  return;
              } else {
                if (root_lock.try_lock([=] {
                  if (root.load() != rt)
                    return false;
                  root = (rt->right).load();
                  node_pool.retire(curr);
                  return true;
                }))
                  return;
              }
            } else {  // subtree with one child
              if (type == LEFT) {
                if ((curr->left).load()) {
                  if (prev->try_lock([=] {
                    if ((prev->left).load() != curr)
                      return false;
                    prev->left = (curr->left).load();
                    node_pool.retire(curr);
                    return true;
                  }))
                    return;
                  goto retry;
                } else {
                  if (prev->try_lock([=] {
                    if ((prev->left).load() != curr)
                      return false;
                    prev->left = (curr->right).load();
                    node_pool.retire(curr);
                    return true;
                  }))
                    return;
                }
              } else {
                if ((curr->left).load()) {
                  if (prev->try_lock([=] {
                    if ((prev->right).load() != curr)
                      return false;
                    prev->right = (curr->left).load();
                    node_pool.retire(curr);
                    return true;
                  }))
                    return;
                } else {
                  if (prev->try_lock([=] {
                    if ((prev->right).load() != curr)
                      return false;
                    prev->right = (curr->right).load();
                    node_pool.retire(curr);
                    return true;
                  }))
                    return;
                }
              }

            }
          }
        }
        prev = curr;
        if (value < (curr->value).load()) {
          curr = (curr->left).load();
          type = LEFT;
        } else {
          curr = (curr->right).load();
          type = RIGHT;
        }
      }
    });
  }

  int get_min() {
    return get_min(root.load());
  }

  int get_min(Node *_root) {
    auto curr = _root;
    auto min = _root ? (_root->value).load() : sentinel_max;

    while (curr) {
      int cv = (curr->value).load();
      if (cv < min) min = cv;

      Node *cl = (curr->left).load();
      Node *cr;
      if (cl) {
        curr = cl;
      } else if ((cr = (curr->right).load())) {
        curr = cr;
      } else
        curr = nullptr;
    }
    return min;
  }

  int get_max() {
    auto curr = root.load();
    auto max = curr ? (curr->value).load() : sentinel_min;

    while (curr) {
      int cv = (curr->value).load();
      if (cv > max) max = cv;

      Node *cl;
      Node *cr = (curr->right).load();
      if (cr) {
        curr = cr;
      } else if ((cl = (curr->left).load())) {
        curr = cl;
      } else
        curr = nullptr;
    }
    return max;
  }

  void print() {
    std::function<void(Node *)> prec;
    prec = [&](Node *p) {
      if (!p)
        return;
      std::cout << (p->value).load() << ", ";
      prec((p->left).load());
      prec((p->right).load());
    };
    prec(root.load());
    std::cout << std::endl;
  }

};

}  // namespace flock_mcas
