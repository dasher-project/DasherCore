#include "SimpleTimer.h"
using namespace std::chrono;

CSimpleTimer::CSimpleTimer()
{
    start = steady_clock::now();
}

CSimpleTimer::~CSimpleTimer()
= default;

double CSimpleTimer::GetElapsed() const
{
	const steady_clock::time_point end = steady_clock::now();
	const auto span = duration_cast<duration<double>>(end - start);
	return span.count();
}

