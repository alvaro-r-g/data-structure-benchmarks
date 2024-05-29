// Original code from Patel et al. 2017 paper
// A hardware implementation of the MCAS synchronization primitive

#pragma once

#include <climits>
#include <memory>
#include "mcas.h"
#include <iostream>
#include <functional>

namespace lockfree_mcas {

class BinarySearchTree {
 private:
  struct Node {
    int value;
    Node* left;
    Node* right;
    Node() = default;
  };

  Node* root;
  int sentinel_min = INT_MIN;
  int sentinel_max = INT_MAX;

  typedef enum {
    LEFT,
    RIGHT,
  } node_type;

 public:
  BinarySearchTree() : root(nullptr) {};

  void insert(const int value) {
    Node *new_node = new Node();
    new_node->value = value;

    while (!root) {
      {
        Node *temp = nullptr;
        // std::lock_guard<std::mutex> lock(cas_lock);
        if (cas(reinterpret_cast<uint64_t *>(&root),
                reinterpret_cast<uint64_t>(temp),
                reinterpret_cast<uint64_t>(new_node)))
          return;
      }
    }

    while (true) {
      Node *curr = root;
      Node *prev = nullptr;
      node_type type = LEFT;

      while (curr) {
        prev = curr;
        if (value < curr->value) {
          curr = curr->left;
          type = LEFT;
        } else {
          curr = curr->right;
          type = RIGHT;
        }
      }
      if (type == LEFT) {
        auto last = prev->left;
        {
          // std::lock_guard<std::mutex> lock(cas_lock);
          if (cas(reinterpret_cast<uint64_t *>(&prev->left),
                  reinterpret_cast<uint64_t>(last),
                  reinterpret_cast<uint64_t>(new_node)))
            return;
        }
      } else {
        auto last = prev->right;
        {
          // std::lock_guard<std::mutex> lock(cas_lock);
          if (cas(reinterpret_cast<uint64_t *>(&prev->right),
                  reinterpret_cast<uint64_t>(last),
                  reinterpret_cast<uint64_t>(new_node)))
            return;
        }
      }
    }
  }

  void remove(int value) {
    retry:
    Node *curr = root;
    Node *prev = nullptr;
    node_type type = LEFT;
    while (curr) {
      if (curr->value == value) {
        if (!curr->left && !curr->right) {  // node to be removed has no children’s
          if (curr != root && prev) {      // delete leaf node
            if (type == LEFT) {
              while (true) {
                // Node *last = prev;
                Node *present = curr;
                Node *temp = nullptr;
                {
                  if (dcas(reinterpret_cast<uint64_t *>(&curr),
                           reinterpret_cast<uint64_t>(present),
                           reinterpret_cast<uint64_t>(temp),
                           reinterpret_cast<uint64_t *>(&prev->left),
                           reinterpret_cast<uint64_t>(present),
                           reinterpret_cast<uint64_t>(temp)
                           )) return;
                }
                goto retry;
              }
            } else {
              while (true) {
                // Node *last = prev;
                Node *present = curr;
                Node *temp = nullptr;
                {
                  if (dcas(reinterpret_cast<uint64_t *>(&curr),
                           reinterpret_cast<uint64_t>(present),
                           reinterpret_cast<uint64_t>(temp),
                           reinterpret_cast<uint64_t *>(&prev->right),
                           reinterpret_cast<uint64_t>(present),
                           reinterpret_cast<uint64_t>(temp)))
                    return;
                }
                goto retry;
              }
            }
          } else {
            Node *last = root;
            Node *temp = nullptr;
            //auto temp = dummy->left;
            {
              if (cas(reinterpret_cast<uint64_t *>(&root),
                      reinterpret_cast<uint64_t>(last),
                      reinterpret_cast<uint64_t>(temp)))
                return;
            }
          }  // deleted node is root
        } else if (curr->left &&
                   curr->right) {  // node to be removed has two children’s
          curr->value = get_min(curr->right);  // find minimum value from right subtree
          value = curr->value;
          prev = curr;
          curr = curr->right;  // continue from right subtree delete min node
          type = RIGHT;
          continue;
        } else {              // node to be removed has one children
          if (curr == root) {  // root with one child
            if (root->left) {
              auto last = root;
              auto temp = root->left;
              {
                if (cas(reinterpret_cast<uint64_t *>(&root),
                        reinterpret_cast<uint64_t>(last),
                        reinterpret_cast<uint64_t>(temp)))
                  return;
              }
            } else {
              auto last = root;
              auto temp = root->right;
              {
                if (cas(reinterpret_cast<uint64_t *>(&root),
                        reinterpret_cast<uint64_t>(last),
                        reinterpret_cast<uint64_t>(temp)))
                  return;
              }
            }
          } else {  // subtree with one child
            if (type == LEFT) {
              if (curr->left) {
                while (true) {
                  Node *present = curr;
                  Node *temp = nullptr;
                  {
                    if (dcas(reinterpret_cast<uint64_t *>(&prev->left),
                             reinterpret_cast<uint64_t>(present),
                             reinterpret_cast<uint64_t>(present->left),
                             reinterpret_cast<uint64_t *>(&curr),
                             reinterpret_cast<uint64_t>(present),
                             reinterpret_cast<uint64_t>(temp)))
                      return;
                  }
                  goto retry;
                }
              } else {
                Node *present = curr;
                Node *temp = nullptr;
                {
                  if (dcas(reinterpret_cast<uint64_t *>(&prev->left),
                           reinterpret_cast<uint64_t>(present),
                           reinterpret_cast<uint64_t>(present->right),
                           reinterpret_cast<uint64_t *>(&curr),
                           reinterpret_cast<uint64_t>(present),
                           reinterpret_cast<uint64_t>(temp))) return;
                }
              }
            } else {
              if (curr->left) {
                Node *last = curr->left;
                Node *present = curr;
                Node *temp = nullptr;
                {
                  if (dcas(reinterpret_cast<uint64_t *>(&prev->right),
                           reinterpret_cast<uint64_t>(present),
                           reinterpret_cast<uint64_t>(last),
                           reinterpret_cast<uint64_t *>(&curr),
                           reinterpret_cast<uint64_t>(present),
                           reinterpret_cast<uint64_t>(temp))) return;
                }
              } else {
                Node *last = curr->right;
                Node *present = curr;
                Node *temp = nullptr;
                {
                  if(dcas(reinterpret_cast<uint64_t *>(&prev->right),
                          reinterpret_cast<uint64_t>(present),
                          reinterpret_cast<uint64_t>(last),
                          reinterpret_cast<uint64_t *>(&curr),
                          reinterpret_cast<uint64_t>(present),
                          reinterpret_cast<uint64_t>(temp)))
                    return;
                }
              }
            }
          }
        }
      }
      prev = curr;
      if (value < curr->value) {
        curr = curr->left;
        type = LEFT;
      } else {
        curr = curr->right;
        type = RIGHT;
      }
    }
  }

  int get_min() {
    return get_min(root);
  }

  int get_min(Node *_root) {
    if (!_root)
      return sentinel_max;

    auto curr = _root;

    while (curr->left) {
      curr = curr->left;
    }

    return curr->value;
  }

  void print() {
    std::function<void(Node *)> prec;
    prec = [&](Node *p) {
      if (!p)
        return;
      std::cout << p->value << ", ";
      prec(p->left);
      prec(p->right);
    };
    prec(root);
    std::cout << std::endl;
  }
};

}  // namespace lockfree_mcas
