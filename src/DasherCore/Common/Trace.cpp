// Trace.cpp
//
// Copyright (c) 2005 David Ward

#include "Trace.h"
#include <cstdio>
#include <iostream>

// Track memory leaks on Windows to the line that new'd the memory
#ifdef _WIN32
#ifdef _DEBUG
#define DEBUG_NEW new (_NORMAL_BLOCK, THIS_FILE, __LINE__)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

// Customize behaviour of Trace here

// Send Trace to stdout
void DasherTraceOutputImpl(const char* pszFormat, va_list vargs) {
    vfprintf(stdout, pszFormat, vargs);
}
