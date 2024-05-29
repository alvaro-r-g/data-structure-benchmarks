// Adapted from lockbased/BinarySearchTree.h

// (note that this implementation is far from optimal, as it uses coarse-grained
// locks with Flock)

#pragma once

#include <climits>
#include "flock/flock.h"

namespace lockfree_flock {

class BinarySearchTree {
 private:
  struct Node {
    flck::atomic<int> value;
    flck::atomic<Node *> left;
    flck::atomic<Node *> right;
    Node() = default;
    Node(int value) : value(value), left(nullptr), right(nullptr) {};
  };

  flck::atomic<Node*> root;
  int sentinel_min = INT_MIN;
  int sentinel_max = INT_MAX;

  flck::lock bst_lock;
  flck::memory_pool<Node> node_pool;

  typedef enum {
    LEFT,
    RIGHT,
  } node_type;

 public:
  BinarySearchTree() : root(nullptr) {};

  void insert(const int value) {
    flck::with_epoch([=] {
      bst_lock.with_lock([=] {
        Node *new_node = node_pool.new_obj(value);

        if (!root.load()) {
          root = new_node;
          return true;
        }

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
          prev->left = new_node;
        } else {
          prev->right = new_node;
        }
        return true;
      });
    });
  }

  void remove(int data) {
    flck::with_epoch([=] {
      bst_lock.with_lock([=] {
        int value = data;
        // Because of coarse-grained lock, only one thread can access the tree,
        // so I can pre-load and reuse some values without checking if they
        // changed
        Node *rt = root.load();
        Node *curr = rt;
        Node *prev = nullptr;
        node_type type = LEFT;
        while (curr) {
          Node *cl = (curr->left).load();
          Node *cr = (curr->right).load();

          if ((curr->value).load() == value) {
            if (!cl && !cr) { // node to be removed has no children’s
              if (curr != rt && prev) { // delete leaf node
                if (type == LEFT)
                  prev->left = nullptr;
                else
                  prev->right = nullptr;
              } else root = nullptr; // deleted node is root
            } else if (cl && cr) { // node to be removed has two children’s
              curr->value = get_min_UNSAFE(cr); // find minimum value from right subtree
              value = (curr->value).load();
              prev = curr;
              curr = cr; // continue from right subtree delete min node
              type = RIGHT;
              continue;
            } else { // node to be removed has one children
              if (curr == rt) { // root with one child
                Node *rtl = (rt->left).load();
                root = rtl ? rtl : (rt->right).load();
              } else { // subtree with one child
                if (type == LEFT)
                  prev->left = cl ? cl : cr;
                else
                  prev->right = cl ? cl : cr;
              }
            }
          }
          prev = curr;
          if (value < (curr->value).load()) {
            curr = cl;
            type = LEFT;
          } else {
            curr = cr;
            type = RIGHT;
          }
        }
        return true;
      });
    });
  }

  int get_min() {
    return flck::with_epoch([=] {
      return bst_lock.with_lock([=] {
        int min = get_min_UNSAFE(root.load());
        return min;
      });
    });
  }

  int get_min(Node *_root) {
    return flck::with_epoch([=] {
      return bst_lock.with_lock([=] {
        int min = get_min_UNSAFE(_root);
        return min;
      });
    });
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

 private:

  int get_min_UNSAFE(Node *_root) {
    if (!_root) return sentinel_max;

    Node *curr = _root;
    Node * cl;

    while ((cl = (curr->left).load())) {
      curr = cl;
    }

    return (curr->value).load();
  }
  
};

}  // namespace lockfree_flock
