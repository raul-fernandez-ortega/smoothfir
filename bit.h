/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _BIT_H_
#define _BIT_H_

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"

static inline bool bit_isset(const uint32_t bitset[], int pos)
{
  int n = pos >> 5;
  return bitset[n] & 1 << (pos - (n << 5));
}

static inline void bit_set(uint32_t bitset[], int pos)
{
  int n = pos >> 5;
  bitset[n] = bitset[n] | 1 << (pos - (n << 5));
}

static inline void bit_clr(uint32_t bitset[], int pos)
{
  int n = pos >> 5;
  bitset[n] = bitset[n] & ~(1 << (pos - (n << 5)));
}

static inline bool bit_isset_volatile(volatile const uint32_t bitset[], int pos)
{
  int n = pos >> 5;
  return bitset[n] & 1 << (pos - (n << 5));
}

static inline void bit_set_volatile(volatile uint32_t bitset[], int pos)
{
  int n = pos >> 5;
  bitset[n] = bitset[n] | 1 << (pos - (n << 5));
}

static inline void bit_clr_volatile(volatile uint32_t bitset[], int pos)
{
  int n = pos >> 5;
  bitset[n] = bitset[n] & ~(1 << (pos - (n << 5)));
}

#if defined(__ARCH_IA32__)

static inline int bit_bsf(uint32_t n)
{
  int r;
  __asm__ volatile ("bsfl %1,%0\n\t" : "=r" (r) : "g" (n));
  return r;
}

#else

static inline int bit_bsf(uint32_t n)
{
  static const int tab[256] = {
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
  };
  if ((n & 0x0000FFFF) != 0) {
    if ((n & 0x000000FF) != 0) {
      return tab[n & 0x000000FF];
    } else {
      return 8 + tab[(n & 0x0000FF00) >> 8];
    }
  } else {
    if ((n & 0x00FF0000) != 0) {
      return 16 + tab[(n & 0x00FF0000) >> 16];
    } else {
      return 24 + tab[(n & 0xFF000000) >> 24];
    }
  }    
}

#endif

static inline int bit_find(const uint32_t bitset[], int start, int end)
{
  uint32_t b;
  int n;
  
  if (end < start) {
    return -1;
  }
  if ((b = bitset[start >> 5] >> (start & 0x1F)) != 0) {
    if ((n = bit_bsf(b) + start) > end) {
      return -1;
    }
    return n;
  }
  for (n = (start >> 5) + 1; n <= end >> 5; n++) {
    if (bitset[n] != 0) {
      if ((n = bit_bsf(bitset[n]) + (n << 5)) > end) {
	return -1;
      }
      return n;
    }
  }
  return -1;
}

static inline void field_repeat_test(uint32_t *bitset, const char name[], int bit)
{
  if (bit_isset(bitset, bit)) {
    fprintf(stderr,"Field is already set:%s\n",name);
    exit(SF_EXIT_OTHER);
  }    
  bit_set(bitset, bit);
}

static inline void field_mandatory_test(uint32_t bitset, uint32_t bits, const char name[])
{
  char msg[200];
    
  if ((bitset & bits) != bits) {
    sprintf(msg, "At least one mandatory field is missing in %s.\n", name);
    fprintf(stderr,"%s",msg);
    exit(SF_EXIT_OTHER);
  }
}


#endif
