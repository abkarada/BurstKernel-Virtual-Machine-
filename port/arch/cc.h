#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#define LWIP_NO_CTYPE_H 1

#include <stdint.h>
#include <stddef.h>

typedef uint8_t  u8_t;
typedef int8_t   s8_t;
typedef uint16_t u16_t;
typedef int16_t  s16_t;
typedef uint32_t u32_t;
typedef int32_t  s32_t;
typedef uintptr_t mem_ptr_t;

#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"

#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

extern void puts_serial(const char *s);

#define LWIP_PLATFORM_DIAG(x) do { } while(0)
#define LWIP_PLATFORM_ASSERT(x) do { puts_serial("LWIP Assertion failed: "); puts_serial(x); puts_serial("\n"); while(1); } while(0)

#define lwip_isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define lwip_isdigit(c) ((c) >= '0' && (c) <= '9')
#define lwip_isxdigit(c) (lwip_isdigit(c) || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#define lwip_isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r' || (c) == '\f' || (c) == '\v')
#define lwip_islower(c) ((c) >= 'a' && (c) <= 'z')

#endif // LWIP_ARCH_CC_H
