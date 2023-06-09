#include "breitlingSolver.h"

#include "../geometry.h"

#include <assert.h>
#include <set>

namespace breitling_constraints {

bool isStationInMandatoryRegion(const Station &station, size_t region)
{
  static_assert(MANDATORY_REGION_COUNT == 4);
  switch (region) {
  case 0: return station.getLocation().lon < -1.66;
  case 1: return station.getLocation().lon < 2 && station.getLocation().lat < 44.5;
  case 2: return station.getLocation().lon > 5 && station.getLocation().lat < 44.5;
  case 3: return station.getLocation().lon > 6 && station.getLocation().lat > 46.5;
  default: assert(false); return false;
  }
}

size_t getStationRegion(const Station &station)
{
  for (size_t r = 0; r < MANDATORY_REGION_COUNT; r++) {
    if (isStationInMandatoryRegion(station, r))
      return r;
  }
  return -1;
}

bool satisfiesRegionsConstraints(const ProblemPath &path)
{
    static_assert(MANDATORY_REGION_COUNT == 4);
    static_assert(sizeof(uint32_t) == 4);
    union {
        bool notCrossedRegions[MANDATORY_REGION_COUNT];
        uint32_t anyRegionsNotCrossed;
    };
    anyRegionsNotCrossed = 0;
    notCrossedRegions[0] = notCrossedRegions[1] = notCrossedRegions[2] = notCrossedRegions[3] = true;
    for(size_t i = 0; i < path.size() && anyRegionsNotCrossed; i++) {
        const ProblemStation &s = path[i];
        for(size_t r = 0; r < MANDATORY_REGION_COUNT; r++)
            notCrossedRegions[r] = notCrossedRegions[r] && !isStationInMandatoryRegion(*s.getOriginalStation(), r);
    }
    return anyRegionsNotCrossed == 0;
}

bool satisfiesStationCountConstraints(const ProblemPath &path)
{
    std::set<ProblemStation> distinctStations(path.begin(), path.end());
    return distinctStations.size() >= MINIMUM_STATION_COUNT;
}

bool satisfiesPathConstraints(const ProblemMap &map, const BreitlingData &dataset, const ProblemPath &path)
{
    return path.size() > 0
      && path[0] == map[dataset.departureStation] 
      && (dataset.targetStation == BreitlingData::NO_SPECIFIED_STATION
          || path[path.size() - 1] == map[dataset.targetStation]);
}

bool satisfiesFuelConstraints(const BreitlingData &dataset, const ProblemPath &path)
{
    assert(dataset.planeFuelUsage > 0);
    assert(dataset.planeSpeed > 0);
    assert(dataset.planeFuelCapacity > 0);
    nauticmiles_t currentDistance = 0;
    nauticmiles_t distanceSinceLastRefuel = 0;
    for (size_t i = 1; i < path.size(); i++) {
        const ProblemStation &station = path[i];
        nauticmiles_t flightDistance = geometry::distance(path[i - 1].getLocation(), station.getLocation());
        currentDistance += flightDistance;
        distanceSinceLastRefuel += flightDistance;
        if (!station.canBeUsedToFuel()) {
            float remainingFuel = dataset.planeFuelCapacity - distanceSinceLastRefuel / dataset.planeSpeed * dataset.planeFuelUsage;
            if (remainingFuel < 0)
                return false;
        } else {
            distanceSinceLastRefuel = 0;
        }
    }
    return true;
}

bool satisfiesTimeConstraints(const BreitlingData &dataset, const ProblemPath &path)
{
    daytime_t totalTime = getLength(path) / dataset.planeSpeed;
    return totalTime < MAXIMUM_FLYGHT_DURATION;
}

} // !namespace breitling_constraints

