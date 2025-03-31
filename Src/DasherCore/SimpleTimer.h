
// Simple millisecond accurate timer.
//
// Copyright 2004 by Keith Vertanen

#pragma once

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


