#pragma once

#include <pthread.h>
#include <cassert>
#include <cstdint>
#include <memory>

namespace lockbased {
namespace ArraySwap {

const int NUM_SUB_ITEMS = 2;
const int NUM_ROWS = 100000;

struct Element {
  int32_t value_[NUM_SUB_ITEMS];
};

struct Datum {
  // pointer to the hashmap
  Element* elements_;
  // A lock which protects this Datum
  pthread_mutex_t* lock_;
};

struct sps {
  Datum* array;
  int num_rows_;
  int num_sub_items_;
};

sps* S;

void datum_init(sps* s) {
  for (int i = 0; i < NUM_ROWS; i++) {
    s->array[i].elements_ = (Element*)malloc(sizeof(Element));
    s->array[i].lock_ = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(s->array[i].lock_, nullptr);

    for (int j = 0; j < NUM_SUB_ITEMS; j++)
      s->array[i].elements_->value_[j] = i + j;
  }
}

void datum_free(sps* s) {
  for (int i = 0; i < NUM_ROWS; i++) {
    free(s->array[i].elements_);
    free(s->array[i].lock_);
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

  pthread_mutex_lock(S->array[index_a].lock_);
  pthread_mutex_lock(S->array[index_b].lock_);

  // swap array values
  Element* temp;
  temp = (S->array[index_a].elements_);
  (S->array[index_a].elements_) = (S->array[index_b].elements_);
  (S->array[index_b].elements_) = temp;

  pthread_mutex_unlock(S->array[index_a].lock_);
  pthread_mutex_unlock(S->array[index_b].lock_);

  return true;
}

}  // namespace ArraySwap
}  // namespace lockbased
