#pragma once

#include <stdint.h>


#ifdef MCAS_GEM5

inline __attribute__ ((always_inline)) uint64_t cas(uint64_t* addr0, uint64_t old0, uint64_t new0) {
  register uint64_t rax asm("rax") = (uint64_t)addr0;
  register uint64_t rdx asm("rdx") = (uint64_t)old0;
  register uint64_t rcx asm("rcx") = (uint64_t)new0;

  asm ("\
        .byte 0xf0, 0x48, 0x69, 0xc0, 0x00, 0x00, 0x00, 0x80\n\
       " : "=a"(rax) : "r"(rax), "r"(rdx), "r"(rcx) :);

  return rax;
}

inline __attribute__ ((always_inline)) uint64_t dcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
                                                     uint64_t* addr1, uint64_t old1, uint64_t new1) {
  register uint64_t rax asm("rax") = (uint64_t)addr0;
  register uint64_t rdx asm("rdx") = (uint64_t)old0;
  register uint64_t rcx asm("rcx") = (uint64_t)new0;
  register uint64_t rsi asm("rsi") = (uint64_t)addr1;
  register uint64_t rdi asm("rdi") = (uint64_t)old1;
  register uint64_t r8 asm("r8") = (uint64_t)new1;

  asm ("\
        .byte 0xf0, 0x48, 0x69, 0xc0, 0x01, 0x00, 0x00, 0x80\n\
       " : "=a"(rax) : "r"(rax), "r"(rdx), "r"(rcx), "r"(rsi), "r"(rdi), "r"(r8) :);

  return rax;
}

inline __attribute__ ((always_inline)) uint64_t tcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
                                                     uint64_t* addr1, uint64_t old1, uint64_t new1,
                                                     uint64_t* addr2, uint64_t old2, uint64_t new2) {
  register uint64_t rax asm("rax") = (uint64_t)addr0;
  register uint64_t rdx asm("rdx") = (uint64_t)old0;
  register uint64_t rcx asm("rcx") = (uint64_t)new0;
  register uint64_t rsi asm("rsi") = (uint64_t)addr1;
  register uint64_t rdi asm("rdi") = (uint64_t)old1;
  register uint64_t r8 asm("r8") = (uint64_t)new1;
  register uint64_t r9 asm("r9") = (uint64_t)addr2;
  register uint64_t r10 asm("r10") = (uint64_t)old2;
  register uint64_t r11 asm("r11") = (uint64_t)new2;

  asm ("\
        .byte 0xf0, 0x48, 0x69, 0xc0, 0x02, 0x00, 0x00, 0x80\n\
       " : "=a"(rax) : "r"(rax), "r"(rdx), "r"(rcx), "r"(rsi), "r"(rdi), "r"(r8), "r"(r9), "r"(r10), "r"(r11) :);

  return rax;
}

inline __attribute__ ((always_inline)) uint64_t qcas(uint64_t* addr0, uint64_t old0, uint64_t new0,
                                                     uint64_t* addr1, uint64_t old1, uint64_t new1,
                                                     uint64_t* addr2, uint64_t old2, uint64_t new2,
                                                     uint64_t* addr3, uint64_t old3, uint64_t new3) {

  register uint64_t rax asm("rax") = (uint64_t)addr0;
  register uint64_t rdx asm("rdx") = (uint64_t)old0;
  register uint64_t rcx asm("rcx") = (uint64_t)new0;
  register uint64_t rsi asm("rsi") = (uint64_t)addr1;
  register uint64_t rdi asm("rdi") = (uint64_t)old1;
  register uint64_t r8 asm("r8") = (uint64_t)new1;
  register uint64_t r9 asm("r9") = (uint64_t)addr2;
  register uint64_t r10 asm("r10") = (uint64_t)old2;
  register uint64_t r11 asm("r11") = (uint64_t)new2;
  register uint64_t r12 asm("r12") = (uint64_t)addr3;
  register uint64_t r13 asm("r13") = (uint64_t)old3;
  register uint64_t r14 asm("r14") = (uint64_t)new3;

  asm ("\
        .byte 0xf0, 0x48, 0x69, 0xc0, 0x03, 0x00, 0x00, 0x80\n\
       " : "=a"(rax) : "r"(rax), "r"(rdx), "r"(rcx), "r"(rsi), "r"(rdi), "r"(r8), "r"(r9), "r"(r10), "r"(r11),  "r"(r12), "r"(r13), "r"(r14) :);

  return rax;
}

#else

#include "../../configuration.h"
#include "mcas-lock.h"
#include "mcas-lock-no-if.h"
#include "mcas-htm.h"
#include "mcas-htm-no-if.h"

uint64_t (*cas)(uint64_t*, uint64_t, uint64_t);

uint64_t (*dcas)(uint64_t*, uint64_t, uint64_t,
                 uint64_t*, uint64_t, uint64_t);

uint64_t (*tcas)(uint64_t*, uint64_t, uint64_t,
                 uint64_t*, uint64_t, uint64_t,
                 uint64_t*, uint64_t, uint64_t);

uint64_t (*qcas)(uint64_t*, uint64_t, uint64_t,
                 uint64_t*, uint64_t, uint64_t,
                 uint64_t*, uint64_t, uint64_t,
                 uint64_t*, uint64_t, uint64_t);

void set_mcas_type(Configuration::SyncType type) {
  switch (type)
  {
  case Configuration::SyncType::LOCKFREE_MCAS:
    cas = mcas_lock::cas;
    dcas = mcas_lock::dcas;
    tcas = mcas_lock::tcas;
    qcas = mcas_lock::qcas;
    break;
  case Configuration::SyncType::MCAS_NO_IF:
    cas = mcas_lock_no_if::cas;
    dcas = mcas_lock_no_if::dcas;
    tcas = mcas_lock_no_if::tcas;
    qcas = mcas_lock_no_if::qcas;
    break;
  case Configuration::SyncType::MCAS_HTM:
    cas = mcas_htm::cas;
    dcas = mcas_htm::dcas;
    tcas = mcas_htm::tcas;
    qcas = mcas_htm::qcas;
    break;
  case Configuration::SyncType::MCAS_HTM_NO_IF:
    cas = mcas_htm_no_if::cas;
    dcas = mcas_htm_no_if::dcas;
    tcas = mcas_htm_no_if::tcas;
    qcas = mcas_htm_no_if::qcas;
    break;
  default:
    break;
  }
}

#endif
