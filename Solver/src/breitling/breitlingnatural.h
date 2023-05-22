#pragma once

#include <list>
#include <algorithm>

#include "breitlingSolver.h"
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

  struct ResolutionState {
    std::vector<std::vector<ProblemStation>> closedStations;
    disttime_t currentTime = 0;
    disttime_t remainingFuel = 0;
    size_t targetIdx = 0;
    ProblemPath path;
  };

private:
  BreitlingData m_dataset;
  disttime_t m_planeCapacity;
  
public:
  NaturalBreitlingSolver(const BreitlingData &dataset)
    : m_dataset(dataset), m_planeCapacity(dataset.planeFuelCapacity / dataset.planeFuelUsage)
  {
  }

  virtual ProblemPath solveForPath(const ProblemMap &map) override;

private:
  std::vector<PathTarget> generateTargets(const ProblemMap &map);
  const ProblemStation *nearestAccessible(const ProblemMap &map, const ResolutionState &state, Location location);

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

  inline bool doesPathCoverTarget(const ProblemPath &path, const PathTarget &target)
  {
    for (const ProblemStation &station : path)
      if (getTimeDistance(station.getLocation(), target.location) < target.radius)
        return true;
    return false;
  }
};