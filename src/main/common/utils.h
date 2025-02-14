#pragma once

#include <stddef.h>
#include <stdint.h>

#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))
#define ARRAYEND(x) (&(x)[ARRAYLEN(x)])

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

static inline int popcount(unsigned x) { return __builtin_popcount(x); }
static inline int popcount32(uint32_t x) { return __builtin_popcount(x); }
static inline int popcount64(uint64_t x) { return __builtin_popcountll(x); }

static inline int32_t cmp32(uint32_t a, uint32_t b) { return (int32_t)(a-b); }
