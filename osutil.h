#ifndef OSUTIL_HH
#define OSUTIL_HH

#include <string.h>

void fatal(const char *fmt, ...);
void *mallocsafe(size_t size);
void *strdupsafe(char *str);

#if (BYTE_ORDER == LITTLE_ENDIAN)
#define SWAP_BE_16(u16) (((u16 & 0xFF00) >> 8) + ((u16 & 0xFF) << 8))
#define SWAP_LE_16(u16) (u16)
#define SWAP_BE_32(u32) (((u32 & 0xFF000000) >> 24) + ((u32 & 0x00FF0000) >> 8) + ((u32 & 0x0000FF00) << 8) + ((u32 & 0x000000FF) << 24))
#define SWAP_LE_32(u32) (u32)
#else
#define SWAP_LE_16(u16) (((u16 & 0xFF00) >> 8) + ((u16 & 0xFF) << 8))
#define SWAP_BE_16(u16) (u16)
#define SWAP_LE_32(u32) (((u32 & 0xFF000000) >> 24) + ((u32 & 0x00FF0000) >> 8) + ((u32 & 0x0000FF00) << 8) + ((u32 & 0x000000FF) << 24))
#define SWAP_BE_32(u32) (u32)
#endif

#define LOWER_CLAMP(val, bound) { if (val < bound) val = bound; }
#define UPPER_CLAMP(val, bound) { if (val > bound) val = bound; }
#define CLAMP(val, lower, upper) { LOWER_CLAMP(val, lower); UPPER_CLAMP(val, upper); }

#endif
