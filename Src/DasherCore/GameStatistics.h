#pragma once

// Code examples modified from C++ Cookbook O'Reilly.
// Code use falls under fair use as dicussed in the book

#include <numeric>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>

// 1/size(v) * Sum_{i \elem v}((i-mean)^n)
template<class T, class U>
U nthMoment(int n, const std::vector<T> &v, U mean)
{
  return std::accumulate(v.begin(), v.end(), U(), [mean, n](const U &sum, const T &current){return sum+std::pow(current-mean, n);})/v.size();
}

template<typename T>
std::string ComputeStats(const std::vector<T> &v)
{
  if (v.empty()) return "";
  
  double m1 = nthMoment(1, v, 0.0);
  double m2 = nthMoment(2, v, m1);
  double m3 = nthMoment(3, v, m1);
  double m4 = nthMoment(4, v, m1);
  
  double dev = sqrt(m2); // Standard Deviation
  double skew = m3/(m2*dev); // Skewness
  double kurt = m4 / (m2*m2) - 3.0; // Excess Kurtosis

  std::ostringstream m_Statsbreakdown("");  
#define SEP " "
  m_Statsbreakdown << "Samples: " << v.size() << SEP
    << "Mean: " << m1 << SEP
    << "StdDev: " << dev << SEP
    << "Skew: " << skew << SEP
    << "Kurt: " << kurt << SEP;
#undef SEP
  return m_Statsbreakdown.str();
}

template<class A, class B, class T>
struct MemberSumDiffNthPower 
{
  MemberSumDiffNthPower(A T::* pm, B x, int n) : m_mean(x), m_pm(pm), m_n(n) {}
  B operator()(B sum, T current)
  {
    return sum+std::pow(current.*m_pm-m_mean, m_n);
  }
  B m_mean;
  A T::* m_pm;
  int m_n;
};

template<class T, class Iter_T, class A, class B>
  B MemberNthMoment(int n, Iter_T first, Iter_T last, A T::* pmember, B mean)
{
  size_t cnt = last - first;
  return std::accumulate(first, last, B(), MemberSumDiffNthPower<A,B,T>(pmember,mean,n))/cnt;
}

