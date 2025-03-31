// Keeps track of the user's mouse location at a certain point in time.
//
// Can store a integer x, y coordinate and/or a normalized floating
// point x, y pair where (1.0, 1.0) is the lower right corener of the
// window.
//
// Copyright 2005 by Keith Vertanen

#ifndef __USER_LOCATION_H__
#define __USER_LOCATION_H__

#include "FileLogger.h"
#include <string>
#include <vector>

extern CFileLogger* g_pLogger;

class CUserLocation;

typedef std::vector<CUserLocation>               VECTOR_USER_LOCATION;
typedef std::vector<CUserLocation>::iterator     VECTOR_USER_LOCATION_ITER;
typedef std::vector<CUserLocation*>              VECTOR_USER_LOCATION_PTR;
typedef std::vector<CUserLocation*>::iterator    VECTOR_USER_LOCATION_PTR_ITER;

/// \ingroup Logging
/// @{
class CUserLocation
{
public:
  CUserLocation(int iX, int iY, float dNats);
  CUserLocation(float iX, float iY, float dNats);
  CUserLocation(int iX1, int iY1, float iX2, float iY2, float dNats);
  CUserLocation(int iX, int iY, int iTop, int iLeft, int iBottom, int iRight, bool bStoreIntegerRep, float dNats);
  ~CUserLocation();

  std::string              GetXML(const std::string& strPrefix = "");
  static double       ComputeNormalizedX(int iX, int iLeft, int iRight);
  static double       ComputeNormalizedY(int iY, int iTop, int iBottom);

  // Used when we want to post-process a XML log file:
  CUserLocation(const std::string& strXML);
  std::string              GetTabMouseXY(bool bReturnNormalized);
  void                GetMouseGridLocation(int iGridSize, int* pRow, int* pCol);

private:
  std::string              m_strTime;
  int                 m_iLocationX;
  int                 m_iLocationY;
  float               m_dNormalizedLocationX;
  float               m_dNormalizedLocationY;
  bool                m_bHasNormalized;           // Are we storing a normalized representation?
  bool                m_bHasInteger;              // Are we storing an integer representation?
  float               m_dNats;

  void                InitMemeberVars();
};
/// @}

#endif
