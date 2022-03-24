#include "SimpleTimer.h"

CSimpleTimer::CSimpleTimer()
{
    start = std::chrono::steady_clock::now();
}

CSimpleTimer::~CSimpleTimer()
= default;

double CSimpleTimer::GetElapsed() const
{
	const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	const auto span = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	return span.count();
}

