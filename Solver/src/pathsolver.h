#pragma once

#include "geomap.h"
#include "path.h"

struct ProblemStation {
private:
  const Station *m_station;
  bool m_isAccessibleAtNight;
  bool m_canBeUsedToFuel;
public:
  ProblemStation(const Station *station, bool isAccessibleAtNight, bool canBeUsedToFuel)
    : m_station(station), m_isAccessibleAtNight(isAccessibleAtNight), m_canBeUsedToFuel(canBeUsedToFuel)
  {
  }

  ProblemStation() 
    : m_station(nullptr), m_isAccessibleAtNight(false), m_canBeUsedToFuel(false)
  {
  }

  const Location &getLocation() const { return m_station->getLocation(); }
  bool isAccessibleAtNight() const { return m_isAccessibleAtNight; }
  bool canBeUsedToFuel() const { return m_canBeUsedToFuel; }
  const Station *getOriginalStation() const { return m_station; }
};

typedef std::vector<ProblemStation> ProblemMap;

class PathSolver {
public:
    virtual Path solveForPath(const ProblemMap &map) = 0;
};

