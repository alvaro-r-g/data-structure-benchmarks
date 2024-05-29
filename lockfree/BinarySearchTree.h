// Original code from Patel et al. 2017 paper
// A hardware implementation of the MCAS synchronization primitive

#pragma once

#include <climits>
#include <cstdint>
#include <cstdlib>

#include "DPointer.h"

namespace lockfree {

class Node {
 public:
  long key;
  long value;
  DPointer<Node, sizeof(size_t)> lChild;
  DPointer<Node, sizeof(size_t)> rChild;
  Node() {}
  Node(long key, long value) {
    this->key = key;
    this->value = value;
  }
  Node(long key, long value, DPointer<Node, sizeof(size_t)> lChild,
       DPointer<Node, sizeof(size_t)> rChild) {
    this->key = key;
    this->value = value;
    this->lChild = lChild;
    this->rChild = rChild;
  }
};

class SeekRecord {
 public:
  Node *ancestor;
  Node *successor;
  Node *parent;
  Node *leaf;
  SeekRecord() {}
  SeekRecord(Node *ancestor, Node *successor, Node *parent, Node *leaf) {
    this->ancestor = ancestor;
    this->successor = successor;
    this->parent = parent;
    this->leaf = leaf;
  }
};

class BinarySearchTree {
 public:
  static Node *grandParentHead;
  static Node *parentHead;
  static BinarySearchTree *obj;
  BinarySearchTree() { createHeadNodes(); }
  long lookup(long target) {
    Node *node = grandParentHead;
    while (node->lChild.ptr !=
           NULL)  // loop until a leaf or dummy node is reached
    {
      if (target < node->key) {
        node = node->lChild.ptr;
      } else {
        node = node->rChild.ptr;
      }
    }
    if (target == node->key)
      return (1);
    else
      return (0);
  }
  void add(long insertKey) {
    int nthChild;
    Node *node;
    Node *pnode;
    SeekRecord *s;
    while (true) {
      nthChild = -1;
      pnode = parentHead;
      node = parentHead->lChild.ptr;
      while (node->lChild.ptr !=
             NULL)  // loop until a leaf or dummy node is reached
      {
        if (insertKey < node->key) {
          pnode = node;
          node = node->lChild.ptr;
        } else {
          pnode = node;
          node = node->rChild.ptr;
        }
      }
      Node *oldChild = node;
      if (insertKey < pnode->key) {
        nthChild = 0;
      } else {
        nthChild = 1;
      }
      // leaf node is reached
      if (node->key == insertKey) {
        // key is already present in tree. So return
        return;
      }
      Node *internalNode, *lLeafNode, *rLeafNode;
      if (node->key < insertKey) {
        rLeafNode = new Node(insertKey, insertKey);
        internalNode = new Node(insertKey, insertKey,
                                DPointer<Node, sizeof(size_t)>(node, 0),
                                DPointer<Node, sizeof(size_t)>(rLeafNode, 0));
      } else {
        lLeafNode = new Node(insertKey, insertKey);
        internalNode = new Node(node->key, node->key,
                                DPointer<Node, sizeof(size_t)>(lLeafNode, 0),
                                DPointer<Node, sizeof(size_t)>(node, 0));
      }
      if (nthChild == 0) {
        if (pnode->lChild.cas(DPointer<Node, sizeof(size_t)>(internalNode, 0),
                              oldChild)) {
          return;
        } else {
          // insert failed; help the conflicting delete operation
          if (node == pnode->lChild.ptr) {  // address has not changed. So
            // CAS would have failed coz of flag/mark only
            // help other thread with cleanup
            s = seek(insertKey);
            cleanUp(insertKey, s);
          }
        }
      } else {
        if (pnode->rChild.cas(DPointer<Node, sizeof(size_t)>(internalNode, 0),
                              DPointer<Node, sizeof(size_t)>(oldChild, 0))) {
          return;
        } else {
          if (node == pnode->rChild.ptr) {
            s = seek(insertKey);
            cleanUp(insertKey, s);
          }
        }
      }
    }
  }
  void remove(long deleteKey) {
    bool isCleanUp = false;
    SeekRecord *s;
    Node *parent;
    Node *leaf = NULL;
    while (true) {
      s = seek(deleteKey);
      if (!isCleanUp) {
        leaf = s->leaf;
        if (leaf->key != deleteKey) {
          return;
        } else {
          parent = s->parent;
          if (deleteKey < parent->key) {
            if (parent->lChild.cas(DPointer<Node, sizeof(size_t)>(leaf, 2),
                                   leaf)) {
              isCleanUp = true;
              // do cleanup
              if (cleanUp(deleteKey, s)) {
                return;
              }
            } else {
              if (leaf == parent->lChild.ptr) {
                cleanUp(deleteKey, s);
              }
            }
          } else {
            if (parent->rChild.cas(DPointer<Node, sizeof(size_t)>(leaf, 2),
                                   leaf)) {
              isCleanUp = true;
              // do cleanup
              if (cleanUp(deleteKey, s)) {
                return;
              }
            } else {
              if (leaf == parent->rChild.ptr) {
                // help other thread with cleanup
                cleanUp(deleteKey, s);
              }
            }
          }
        }
      } else {
        if (s->leaf == leaf) {
          // do cleanup
          if (cleanUp(deleteKey, s)) {
            return;
          }
        } else {
          // someone helped with my cleanup. So I’m done
          return;
        }
      }
    }
  }
  int setTag(int stamp) {
    switch (stamp)  // set only tag
    {
      case 0:
        stamp = 1;  // 00 to 01
        break;
      case 2:
        stamp = 3;  // 10 to 11
        break;
    }
    return stamp;
  }
  int copyFlag(int stamp) {
    switch (stamp)  // copy only the flag
    {
      case 1:
        stamp = 0;  // 01 to 00
        break;
      case 3:
        stamp = 2;  // 11 to 10
        break;
    }
    return stamp;
  }

  bool cleanUp(long key, SeekRecord *s) {
    Node *ancestor = s->ancestor;
    Node *parent = s->parent;
    Node *oldSuccessor;
    size_t oldStamp;
    Node *sibling;
    size_t siblingStamp;
    if (key < parent->key) {          // xl case
      if (parent->lChild.mark > 1) {  // check if parent to leaf edge is
                                      // already flagged .10 or 11
        // leaf node is flagged for deletion. tag the sibling edge to
        // prevent
        // any modification at this edge now
        sibling = parent->rChild.ptr;
        siblingStamp = parent->rChild.mark;
        siblingStamp = setTag(siblingStamp);  // set only tag
        parent->rChild.cas(
            DPointer<Node, sizeof(size_t)>(sibling, siblingStamp), sibling);
        sibling = parent->rChild.ptr;
        siblingStamp = parent->rChild.mark;
      } else {
        // leaf node is not flagged. So sibling node must have been flagged
        // for deletion
        sibling = parent->lChild.ptr;
        siblingStamp = parent->lChild.mark;
        siblingStamp = setTag(siblingStamp);  // set only tag
        parent->lChild.cas(
            DPointer<Node, sizeof(size_t)>(sibling, siblingStamp), sibling);
        sibling = parent->lChild.ptr;
        siblingStamp = parent->lChild.mark;
      }
    } else {                          // xr case
      if (parent->rChild.mark > 1) {  // check if parent to leaf edge is
                                      // already flagged .10 or 11
        // leaf node is flagged for deletion. tag the sibling edge to
        // prevent
        // any modification at this edge now
        sibling = parent->lChild.ptr;
        siblingStamp = parent->lChild.mark;
        siblingStamp = setTag(siblingStamp);  // set only tag
        parent->lChild.cas(
            DPointer<Node, sizeof(size_t)>(sibling, siblingStamp), sibling);
        sibling = parent->lChild.ptr;
        siblingStamp = parent->lChild.mark;
      } else {
        // leaf node is not flagged. So sibling node must have been flagged
        // for deletion
        sibling = parent->rChild.ptr;
        siblingStamp = parent->rChild.mark;
        siblingStamp = setTag(siblingStamp);  // set only tag
        parent->rChild.cas(
            DPointer<Node, sizeof(size_t)>(sibling, siblingStamp), sibling);
        sibling = parent->rChild.ptr;
        siblingStamp = parent->rChild.mark;
      }
    }
    if (key < ancestor->key) {
      siblingStamp = copyFlag(siblingStamp);  // copy only the flag
      oldSuccessor = ancestor->lChild.ptr;
      oldStamp = ancestor->lChild.mark;
      return (ancestor->lChild.cas(
          DPointer<Node, sizeof(size_t)>(sibling, siblingStamp),
          DPointer<Node, sizeof(size_t)>(oldSuccessor, oldStamp)));
    } else {
      siblingStamp = copyFlag(siblingStamp);  // copy only the flag
      oldSuccessor = ancestor->rChild.ptr;
      oldStamp = ancestor->rChild.mark;
      return (ancestor->rChild.cas(
          DPointer<Node, sizeof(size_t)>(sibling, siblingStamp),
          DPointer<Node, sizeof(size_t)>(oldSuccessor, oldStamp)));
    }
  }

  SeekRecord *seek(long key) {
    DPointer<Node, sizeof(size_t)> parentField;
    DPointer<Node, sizeof(size_t)> currentField;
    Node *current;
    // initialize the seek record
    SeekRecord *s = new SeekRecord(grandParentHead, parentHead, parentHead,
                                   parentHead->lChild.ptr);
    parentField = s->ancestor->lChild;
    currentField = s->successor->lChild;
    while (currentField.ptr != NULL) {
      current = currentField.ptr;
      // move down the tree
      // check if the edge from the current parent node in the access path is
      //       tagged
      if (parentField.mark == 0 || parentField.mark == 2) {  // 00, 10 untagged
        s->ancestor = s->parent;
        s->successor = s->leaf;
      }
      // advance parent and leaf pointers
      s->parent = s->leaf;
      s->leaf = current;
      parentField = currentField;
      if (key < current->key) {
        currentField = current->lChild;
      } else {
        currentField = current->rChild;
      }
    }
    return s;
  }

  void createHeadNodes() {
    long key = LONG_MAX;
    long value = LONG_MIN;
    parentHead = new Node(
        key, value, DPointer<Node, sizeof(size_t)>(new Node(key, value), 0),
        DPointer<Node, sizeof(size_t)>(new Node(key, value), 0));
    grandParentHead =
        new Node(key, value, DPointer<Node, sizeof(size_t)>(parentHead, 0),
                 DPointer<Node, sizeof(size_t)>(new Node(key, value), 0));
  }

  void insert(long key) {
    add(key);
  }

  long get_min() {
    Node *node = grandParentHead;

    while (node->lChild.ptr != NULL) {
      node = node->lChild.ptr;
    }

    return node->key;
  }
};

lockfree::Node *lockfree::BinarySearchTree::grandParentHead = nullptr;
lockfree::Node *lockfree::BinarySearchTree::parentHead = nullptr;
lockfree::BinarySearchTree *lockfree::BinarySearchTree::obj = nullptr;
}  // namespace lockfree
