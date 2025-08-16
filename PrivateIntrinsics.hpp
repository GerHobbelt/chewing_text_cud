
#pragma once

#include <cstdlib>
#include <cstdint>

#if 01

// Visual Studio 2015 (CLv19 x86) and some older versions of CLv14 x64 will optimize
// our loop into a direct call to _memset and this fails to link because we don't use the CRT
#if defined(_MSC_VER) && _MSC_VER+0 >= 1400
typedef unsigned char BYTE;
#  include <intrin.h>
#  if defined(_MSC_FULL_VER) && _MSC_FULL_VER+0 >= 140050727
#    pragma intrinsic(memcmp, memcpy, memset)
//#  else
//extern "C" void __stosb(BYTE*, BYTE, size_t);
#  endif //~ _MSC_FULL_VER >= 140050727
#  if defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64) // __stosb not available under _M_ARM nor _M_ARM64
#    pragma intrinsic(__stosb)
#    define memset(p,c,s) __stosb((BYTE*)(p),(BYTE)(c),(s))
#    pragma intrinsic(memcmp, memcpy)
#  elif defined(_M_ARM) || defined(_M_ARM64) // For _MSC_VER=1914 (VS 15.7.27703.2026/CL 19.14.26430)
//extern "C" void* __cdecl memset(void *d, int v, size_t l) { char *p=(char*)d; while (l-- > 0) *p++=v; return d; }
//#    pragma function(memcmp, memcpy, memset)
#  endif
#endif //~ _MSC_VER

#endif
