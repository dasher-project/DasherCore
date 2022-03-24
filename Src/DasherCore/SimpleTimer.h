
// Simple millisecond accurate timer.
//
// Copyright 2004 by Keith Vertanen

#ifndef __SIMPLE_TIMER_H__
#define __SIMPLE_TIMER_H__

#include <chrono>

/// \ingroup Logging
/// \{
class CSimpleTimer
{
public:
  CSimpleTimer();
  ~CSimpleTimer();

 double GetElapsed() const;

private:
  std::chrono::steady_clock::time_point start;
};
/// \}

#endif

