// Adapted from lockbased/arrayswap.h

#pragma once

#include <memory>
#include "flock/flock.h"

namespace lockfree_flock {
namespace ArraySwap {

const int NUM_SUB_ITEMS = 2;
const int NUM_ROWS = 100000;

struct Element {
  int32_t value_[NUM_SUB_ITEMS];
};

struct Datum {
  // pointer to the hashmap
  flck::atomic<Element *> elements_;
  // A lock which protects this Datum
  flck::lock* lock_;
};

flck::memory_pool<flck::lock> lock_pool;

struct sps {
  Datum* array;
  int num_rows_;
  int num_sub_items_;
};

sps *S;

void datum_init(sps* s) {
  for (int i = 0; i < NUM_ROWS; i++) {
    s->array[i].elements_ = (Element*)malloc(sizeof(Element));
    s->array[i].lock_ = lock_pool.new_obj();

    for (int j = 0; j < NUM_SUB_ITEMS; j++)
      ((s->array[i].elements_).load())->value_[j] = i + j;
  }
}

void datum_free(sps* s) {
  for (int i = 0; i < NUM_ROWS; i++) {
    free((s->array[i].elements_).load());
    lock_pool.retire(s->array[i].lock_);
  }
}

void initialize() {
  S = (sps*)malloc(sizeof(sps));
  S->num_rows_ = NUM_ROWS;
  S->num_sub_items_ = NUM_SUB_ITEMS;
  S->array = (Datum*)malloc(sizeof(Datum) * NUM_ROWS);
  datum_init(S);

  // fprintf(stderr, "Created array at %p\n", (void*)S);
}

bool swap(unsigned int index_a, unsigned int index_b) {
  // check if index is out of array
  assert(index_a < NUM_ROWS && index_b < NUM_ROWS);

  // exit if swapping the same index
  if (index_a == index_b) return true;

  // enforce index_a < index_b
  if (index_a > index_b) {
    unsigned int index_tmp = index_a;
    index_a = index_b;
    index_b = index_tmp;
  }

  flck::with_epoch([=] {
    S->array[index_a].lock_->with_lock([=] {
      S->array[index_b].lock_->with_lock([=] {
        // swap array values
        Element* temp;
        temp = (S->array[index_a].elements_).load();
        (S->array[index_a].elements_) = (S->array[index_b].elements_).load();
        (S->array[index_b].elements_) = temp;
        return true;
      });
      return true;
    });
  });

  return true;
}

}   // namespace ArraySwap
}   // namespace lockfree_flock

