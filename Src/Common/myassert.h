#ifndef __assert_h__
#define __assert_h__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// The DASHER_ASSERT macro causes execution to break into the debugger in DEBUG mode
// In non-debug debug builds - no check is done

#ifdef DEBUG
#ifdef _WIN32

#include <crtdbg.h>
#define DASHER_ASSERT(expr) _ASSERT(expr)

#else 

// POSIX platforms (eg Linux)
#include <cassert>
#define DASHER_ASSERT(expr) assert(expr)

#endif // _WIN32

#else

// Non-debug version (assertions disabled)
#define DASHER_ASSERT(expr) ((void)true)

#endif // DEBUG

/////////////////////////////////////////////////////////////////////////////

// Pointer checking - some CRTs provide functionality to check the integrity of memory

// DASHER_ASSERT_VALIDPTR_RW(p) asserts that a pointer is valid for read and write
// DASHER_ASSERT_VALIDPTR_R(p) asserts that a pointer is valid for read

#if _MSC_VER >= 1300
#define DASHER_ASSERT_VALIDPTR_RW(p)	DASHER_ASSERT(_CrtIsValidPointer(p, sizeof(p), 1))
#define DASHER_ASSERT_VALIDPTR_R(p)		DASHER_ASSERT(_CrtIsValidPointer(p, sizeof(p), 0))
#else

        // Please implement any platform-specific pointer checking

        // Simple check that the pointer is non-null
#define DASHER_ASSERT_VALIDPTR_RW(p)	DASHER_ASSERT(p!=NULL)
#define DASHER_ASSERT_VALIDPTR_R(p)		DASHER_ASSERT(p!=NULL)

#endif

#endif // __assert_h__
