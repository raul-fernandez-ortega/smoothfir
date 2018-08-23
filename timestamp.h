/*
 * (c) Copyright 2000, 2002, 2004, 2006 -- Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include <inttypes.h>
#include <time.h>

#if defined(__ARCH_GENERIC__)
#define TIMESTAMP_NOT_CLOCK_CYCLE_COUNTER
static inline void
timestamp(volatile uint64_t *ts)
{
    *ts = (uint64_t)clock() * 1000;
}
#endif

#ifdef __ARCH_IA64__
static inline void timestamp(volatile uint64_t *ts)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  *ts = ((unsigned long long)lo)|( ((unsigned long long)hi)<<32);
}
#endif 

#ifdef __ARCH_IA32__
static inline void
timestamp(volatile uint64_t *ts)
{
    __asm__ volatile("rdtsc" : "=A"(*ts));
}
#endif

#ifdef __ARCH_SPARC__
static inline void
timestamp(volatile uint64_t *ts)
{
    __asm__ volatile (
	"rd %%tick, %0    \n\t \
	 clruw %0, %1     \n\t \
	 srlx %0, 32, %0  \n\t"
	: "=r"(((volatile uint32_t *)ts)[0]), "=r"(((volatile uint32_t *)ts)[1])
	: "0"(((volatile uint32_t *)ts)[0]), "1"(((volatile uint32_t *)ts)[1]));
}
#endif

#endif
