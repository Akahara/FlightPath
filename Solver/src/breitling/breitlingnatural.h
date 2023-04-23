#pragma once

#include <list>
#include <algorithm>

#include "breitlingsolver.h"
#include "../geometry.h"




class NaturalBreitlingSolver : public PathSolver {
private:
  typedef double disttime_t;
  typedef uint8_t region_t;

  struct PathTarget {
    Location location;
    disttime_t radius;
    size_t expectedStepsToReach;
  };

  struct RegionGravityCenter {
    double accLon = 0;
    double accLat = 0;
    size_t stationCount = 0;
    Location location;
    region_t regionId = -1;
  };

private:
  BreitlingData m_dataset;
  disttime_t m_planeCapacity;

public:
  NaturalBreitlingSolver(const BreitlingData &dataset)
    : m_dataset(dataset), m_planeCapacity(dataset.planeFuelCapacity / dataset.planeFuelUsage)
  {
  }

  virtual Path solveForPath(const ProblemMap &map) override;

private:
  std::list<PathTarget> generateTargets(const ProblemMap &map);
  const ProblemStation *nearestAccessible(const ProblemMap &map, const Path &currentPath, disttime_t remainingFuelCapacity, disttime_t currentTime, Location location);

  inline disttime_t getTimeDistance(const Location &l1, const Location &l2)
  {
    return geometry::distance(l1, l2) / m_dataset.planeSpeed;
  }

  inline bool isTimeInNightPeriod(disttime_t time)
  {
    // distance is analogous to time
    time = fmod(time, 24.f);
    return time < m_dataset.nauticalDaytime || time > m_dataset.nauticalNighttime;
  }
};