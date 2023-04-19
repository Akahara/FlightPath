#include "breitlingnatural.h"

#include <list>
#include <stdexcept>
#include <array>
#include <iostream>

#include "../geometry.h"

typedef uint8_t region_t;

struct PathTarget {
  Location location;
  double radius;
  size_t expectedStepsToReach;
};

struct RegionGravityCenter {
  double accLon = 0;
  double accLat = 0;
  size_t stationCount = 0;
  Location location;
  region_t regionId = -1;
};

constexpr double REGION_CAPTURE_THRESHOLD = .5;

Location interpolateLocations(const Location &l1, const Location &l2, float x)
{
  // linear interpolation, not exact because lon/lat coordinates cannot be interpolated
  // linearly but good enough for now (on France's map lon/lat can almost be interpreted
  // as xy coordinates)
  return Location(l1.lon + (l2.lon-l1.lon)*x, l1.lat + (l2.lat-l1.lat)*x);
}

std::list<PathTarget> generateTargets(const ProblemMap &map, const BreitlingData &dataset)
{
  constexpr size_t regionCount = breitling_constraints::MANDATORY_REGION_COUNT;

  std::vector<region_t> stationsRegions(map.size(), -1);
  std::array<RegionGravityCenter, regionCount> regionsCenters;
  std::list<PathTarget> targets;

  // find regions centers
  for (size_t i = 0; i < map.size(); i++) {
    const ProblemStation &station = map[i];
    for (region_t r = 0; r < regionCount; r++) {
      if (breitling_constraints::isStationInMandatoryRegion(*station.getOriginalStation(), r)) {
        stationsRegions[i] = r;
        regionsCenters[r].accLon += station.getLocation().lon;
        regionsCenters[r].accLat += station.getLocation().lat;
        regionsCenters[r].stationCount++;
        break;
      }
    }
  }
  
  // average all centers
  for (region_t r = 0; r < regionCount; r++) {
    RegionGravityCenter &center = regionsCenters[r];
    if (center.stationCount == 0)
      throw std::runtime_error("A mandatory region is empty");
    center.regionId = r;
    center.location = Location{ center.accLon / center.stationCount, center.accLat / center.stationCount };
  }

  Location currentLocation = map[dataset.departureStation].getLocation();
  nauticmiles_t totalDistance = 0;

  // create targets
  for (size_t i = 0; i < regionCount; i++) {
    // find the next closest region
    for (region_t r = i+1; r < regionCount; r++) {
      if (geometry::distance(currentLocation, regionsCenters[r].location) < geometry::distance(currentLocation, regionsCenters[i].location))
        std::swap(regionsCenters[i], regionsCenters[r]);
    }

    nauticmiles_t minDistToOutsideOfRegion = std::numeric_limits<nauticmiles_t>::max();
    Location nextTarget = regionsCenters[i].location;

    // find the maximum distance to the region's center that can be crossed *without* being able to exit the region
    // that way if a point has a distance that is less than the found one to nextTarget, it must be in the region
    for (size_t j = 0; j < map.size(); j++) {
      const ProblemStation &station = map[j];
      if (stationsRegions[j] != regionsCenters[i].regionId)
        minDistToOutsideOfRegion = std::min(minDistToOutsideOfRegion, geometry::distance(station.getLocation(), nextTarget));
    }

    totalDistance += geometry::distance(currentLocation, nextTarget);
    targets.push_back({ nextTarget, minDistToOutsideOfRegion * REGION_CAPTURE_THRESHOLD, 0 });
    currentLocation = nextTarget;
  }

  // set the target station as a target for the path
  const Location &targetStationLocation = map[dataset.targetStation].getLocation();
  targets.push_back({ targetStationLocation, 0, 0 });
  totalDistance += geometry::distance(currentLocation, targetStationLocation);

  // sets the expectedStepsToReach for each path target
  constexpr size_t totalSteps = breitling_constraints::MINIMUM_STATION_COUNT;
  nauticmiles_t accumulatedDistance = 0;
  currentLocation = map[dataset.departureStation].getLocation();
  for (PathTarget &target : targets) {
    accumulatedDistance += geometry::distance(currentLocation, target.location);
    currentLocation = target.location;
    target.expectedStepsToReach = accumulatedDistance/totalDistance * totalSteps;
  }

  return targets;
}

const ProblemStation *nearestAccessible(const ProblemMap &map, const BreitlingData &dataset, const Path &currentPath, Location location)
{
  nauticmiles_t minDist = std::numeric_limits<nauticmiles_t>::max();
  const ProblemStation *nearestStation = nullptr;

  auto &pathStations = currentPath.getStations();

  for (size_t i = 0; i < map.size(); i++) {
    const ProblemStation &station = map[i];
    nauticmiles_t dist = geometry::distance(location, station.getLocation());
    if (i != dataset.targetStation &&
        dist < minDist &&
        std::find(pathStations.begin(), pathStations.end(), station.getOriginalStation()) == pathStations.end() &&
        true // TODO check for fuel
        ) {
      minDist = dist;
      nearestStation = &station;
    }
  }

  return nearestStation;
}

Path NaturalBreitlingSolver::solveForPath(const ProblemMap &map)
{
  Path path;

  std::list<PathTarget> nextTargets = generateTargets(map, m_dataset);
  Location currentLocation = map[m_dataset.departureStation].getLocation();

  path.getStations().push_back(map[m_dataset.departureStation].getOriginalStation());
  constexpr size_t STEPS = breitling_constraints::MINIMUM_STATION_COUNT;

  for (size_t step = 0; step < STEPS - 2; step++) {
    const PathTarget &nextTarget = nextTargets.front();
    float distanceToTarget = geometry::distance(currentLocation, nextTarget.location);
    Location expectedTarget = interpolateLocations(currentLocation, nextTarget.location, 1.f/std::max(1ull, nextTarget.expectedStepsToReach - step));

    const ProblemStation *nearest = nearestAccessible(map, m_dataset, path, expectedTarget);
    if (nearest == nullptr)
      //backtrack();
      throw std::runtime_error("Broken path");
    path.getStations().push_back(nearest->getOriginalStation());
    currentLocation = nearest->getLocation();
    if (geometry::distance(nextTarget.location, currentLocation) < nextTarget.radius)
      nextTargets.pop_front();
  }

  path.getStations().push_back(map[m_dataset.targetStation].getOriginalStation());

  return path;
}