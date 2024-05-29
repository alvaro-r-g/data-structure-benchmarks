#pragma once

#include <immintrin.h>
#include <stdbool.h>

#include <atomic>
#include <iostream>

using namespace std;

atomic_int fallback_lock;

uint64_t times_fallback;

unsigned long jcong = 380116160;

inline __attribute__ ((always_inline)) void spinlock_init() { atomic_init(&fallback_lock, 0); }

inline __attribute__ ((always_inline)) long spinlock_isLocked() {
  return atomic_load_explicit(&fallback_lock, memory_order_acquire);
}

inline __attribute__ ((always_inline)) void spinlock_whileIsLocked() {
  while (spinlock_isLocked()) {
    _mm_pause();
  }
}

inline __attribute__ ((always_inline)) void spinlock_lock() {
  do {
    spinlock_whileIsLocked();
  } while (atomic_exchange_explicit(&fallback_lock, 1, memory_order_acquire));
  atomic_thread_fence(memory_order_seq_cst);
}

inline __attribute__ ((always_inline)) void spinlock_unlock() {
  atomic_store_explicit(&fallback_lock, 0, memory_order_release);
}

#define TM_THREAD_ENTER() ;
#define TM_THREAD_EXIT() ;

#define MAX_RETRIES 6

void TM_INIT() {
  spinlock_init();
  times_fallback = 0;
}

void TM_BEGIN(int i) {
  int n_tries = 0;
  unsigned status = _XABORT_EXPLICIT;
  bool retry_with_lock = false;

  // cout << "Begin " << i << endl;

  do {
    n_tries++;
    status = _xbegin();
    if (status == _XBEGIN_STARTED) {
      if (!spinlock_isLocked()) {
        return;
      } else {
        _xabort(_XABORT_EXPLICIT);
      }
    }

    spinlock_whileIsLocked();
    bool xplicit = status & _XABORT_EXPLICIT;
    if (xplicit) {
      --n_tries;
    }

    if (!xplicit && !(status & _XABORT_RETRY)) {
      retry_with_lock = true;
    } else if (!retry_with_lock && n_tries >= MAX_RETRIES) {
      retry_with_lock = true;
    }

#ifdef HTM_BACKOFF
    volatile long a[32];
    volatile long b;
    long j;

    unsigned long computed_backoff = 0;
    {
      unsigned long max_backoff;

      if (n_tries > 16) {
        max_backoff = 64 * 1024 + (n_tries - 16);
      } else {
        unsigned long val;
        val = 1;
        val <<= n_tries;
        max_backoff = val;
      }

      computed_backoff = max_backoff;
    }
    long backoff =
        (unsigned long)((jcong = 69069 * jcong + 1234567)) % computed_backoff;
    for (j = 0; j < backoff; j++) {
      b += a[j % 32];
    }
#endif
  } while (!retry_with_lock);

  spinlock_lock();
  times_fallback++;
  return;
}

bool TM_BEGIN_NO_CONFLICT(int i) {
  int n_tries = 0;
  unsigned status = _XABORT_EXPLICIT;
  bool retry_with_lock = false;

  // cout << "Begin " << i << endl;

  do {
    n_tries++;
    status = _xbegin();
    if (status == _XBEGIN_STARTED) {
      if (!spinlock_isLocked()) {
        return true;
      } else {
        _xabort(_XABORT_EXPLICIT);
      }
    }

    bool conflict = status & _XABORT_CONFLICT;
    if (conflict) {
      return false;
    }

    spinlock_whileIsLocked();
    bool xplicit = status & _XABORT_EXPLICIT;
    if (xplicit) {
      --n_tries;
    }

    if (!xplicit && !(status & _XABORT_RETRY)) {
      retry_with_lock = true;
    } else if (!retry_with_lock && n_tries >= MAX_RETRIES) {
      retry_with_lock = true;
    }

#ifdef HTM_BACKOFF
    volatile long a[32];
    volatile long b;
    long j;

    unsigned long computed_backoff = 0;
    {
      unsigned long max_backoff;

      if (n_tries > 16) {
        max_backoff = 64 * 1024 + (n_tries - 16);
      } else {
        unsigned long val;
        val = 1;
        val <<= n_tries;
        max_backoff = val;
      }

      computed_backoff = max_backoff;
    }
    long backoff =
        (unsigned long)((jcong = 69069 * jcong + 1234567)) % computed_backoff;
    for (j = 0; j < backoff; j++) {
      b += a[j % 32];
    }
#endif
  } while (!retry_with_lock);

  spinlock_lock();
  times_fallback++;
  return true;
}

void TM_END(int i) {
  if (spinlock_isLocked()) {
    spinlock_unlock();
  } else {
    _xend();
  }

  // cout << "End " << i << endl;
}
