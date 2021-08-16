#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <cmath>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using b32 = i32; //@Note: Much more practical for struct packing

#define NAN_BOXING

#define QNAN ((u64)0x7ffc000000000000)
#define SIGN_BIT ((uint64_t)0x8000000000000000)

#define TAG_NIL   1
#define TAG_FALSE 2
#define TAG_TRUE  3

/* #define DEBUG_PRINT_CODE */
/* #define DEBUG_TRACE_EXECUTION */

//#define DEBUG_STRESS_GC
#define DEBUG_LOG_GC

#define UINT8_COUNT (UINT8_MAX + 1)

#endif
