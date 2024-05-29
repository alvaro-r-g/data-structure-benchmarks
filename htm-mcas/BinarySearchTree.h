// Adapted from lockfree-mcas/BinarySearchTree.h

#pragma once

#include <climits>
#include <memory>
#include "tm.h"

namespace htm_mcas {

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

    TM_BEGIN(0);

    if (!root) {
      root = new_node;
      TM_END(0);
      return;
    }

    TM_END(0);

    Node *prev = nullptr;
    node_type type = LEFT;

    TM_BEGIN(1);

    Node *curr = root;

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
      prev->left = new_node;
    } else {
      prev->right = new_node;
    }

    TM_END(1);
  }

  void remove(int value) {
    Node *prev = nullptr;
    
    TM_BEGIN(2); // might be possible to divide this section into smaller ones

    Node *curr = root;
    node_type type = LEFT;
    while (curr) {
      if (curr->value == value) {
        if (!curr->left && !curr->right) {  // node to be removed has no children’s
          if (curr != root && prev) {      // delete leaf node
            if (type == LEFT) {
              prev->left = nullptr;
            } else {
              prev->right = nullptr;
            }
          } else {
            root = nullptr;
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
              root = root->left;
            } else {
              root = root->right;
            }
          } else {  // subtree with one child
            if (type == LEFT) {
              if (curr->left) {
                prev->left = curr->left;
              } else {
                prev->left = curr->right;
              }
            } else {
              if (curr->left) {
                prev->right = curr->left;
              } else {
                prev->right = curr->right;
              }
            }
          }
        }
        TM_END(2);
        return;
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

    TM_END(2);
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
};

}  // namespace htm_mcas
