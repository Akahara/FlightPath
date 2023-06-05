#include "breitlingnatural.h"

#include <list>
#include <stdexcept>
#include <array>
#include <iostream>

#include "../geometry.h"

/**
 * The idea of this algorithm is to set "targets" allong an imaginary
 * path that goes through all 4 regions and then iterate to find the
 * next station to go through. When targets are set we can draw straight
 * lines to link targets in order, and divide the resulting path into N
 * segments (N=100 for the breitling cup), at step i we choose the
 * station that can be reached that is the closest to the target point 
 * that is the end of the i-th segment.
 * 
 * That's the big idea, in the implementation the segments are updated
 * on the fly to compensate for when we drift out of the path. And
 * targets are considered to be reached when we get "close enough",
 * targets are the center of each regions and the radius that is 
 * considered close if the largest such that we are certain we are
 * in the region.
 */

/**
 * Reduces the redius for region targets, must be lower than 1.
 * ie. the lower the value is the closer to the the region's center 
 * the computed path will have to go.
 */
static constexpr double REGION_CAPTURE_THRESHOLD = 1;


std::vector<NaturalBreitlingSolver::PathTarget> NaturalBreitlingSolver::generateTargets(const ProblemMap &map)
{
  constexpr size_t regionCount = breitling_constraints::MANDATORY_REGION_COUNT;

  std::vector<region_t> stationsRegions(map.size(), -1);
  std::vector<RegionGravityCenter> regionsCenters(regionCount);
  std::vector<PathTarget> targets;

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

  { // remove targets for regions that we will cross anyway because of start/end points
    region_t startRegion = stationsRegions[m_dataset.departureStation];
    region_t endRegion = stationsRegions[m_dataset.targetStation];
    regionsCenters.erase(
      std::remove_if(regionsCenters.begin(), regionsCenters.end(),
        [startRegion, endRegion](const auto &center) { return center.regionId == startRegion || center.regionId == endRegion; }),
     regionsCenters.end());
  }

  Location currentLocation = map[m_dataset.departureStation].getLocation();
  disttime_t totalDistance = 0;

  // create targets
  for (size_t i = 0; i < regionsCenters.size(); i++) {
    // find the next closest region
    for (region_t r = i+1; r < regionsCenters.size(); r++) {
      if (getTimeDistance(currentLocation, regionsCenters[r].location) < getTimeDistance(currentLocation, regionsCenters[i].location))
        std::swap(regionsCenters[i], regionsCenters[r]);
    }

    nauticmiles_t minDistToOutsideOfRegion = std::numeric_limits<nauticmiles_t>::max();
    Location nextTarget = regionsCenters[i].location;

    // find the maximum distance to the region's center that can be crossed *without* being able to exit the region
    // that way if a point has a distance that is less than the found one to nextTarget, it must be in the region
    for (size_t j = 0; j < map.size(); j++) {
      const ProblemStation &station = map[j];
      if (stationsRegions[j] != regionsCenters[i].regionId)
        minDistToOutsideOfRegion = std::min(minDistToOutsideOfRegion, getTimeDistance(station.getLocation(), nextTarget));
    }

    totalDistance += getTimeDistance(currentLocation, nextTarget);
    targets.push_back({ nextTarget, minDistToOutsideOfRegion * REGION_CAPTURE_THRESHOLD, 0 });
    currentLocation = nextTarget;
  }

  // set the target station as a target for the path
  const Location &targetStationLocation = map[m_dataset.targetStation].getLocation();
  targets.push_back({ targetStationLocation, 0, 0 });
  totalDistance += getTimeDistance(currentLocation, targetStationLocation);

  // sets the expectedStepsToReach for each path target
  constexpr size_t totalSteps = breitling_constraints::MINIMUM_STATION_COUNT;
  nauticmiles_t accumulatedDistance = 0;
  currentLocation = map[m_dataset.departureStation].getLocation();
  for (PathTarget &target : targets) {
    accumulatedDistance += getTimeDistance(currentLocation, target.location);
    currentLocation = target.location;
    target.expectedStepsToReach = accumulatedDistance/totalDistance * totalSteps;
  }

  return targets;
}

const ProblemStation *NaturalBreitlingSolver::nearestAccessible(const ProblemMap &map, const ResolutionState &state, Location location)
{
  disttime_t minDist = std::numeric_limits<nauticmiles_t>::max();
  const ProblemStation *nearestStation = nullptr;

  const auto &pathStations = state.path;
  const auto &closedStations = state.closedStations[pathStations.size()];

  for (size_t i = 0; i < map.size(); i++) {
    const ProblemStation &station = map[i];
    nauticmiles_t dist = getTimeDistance(location, station.getLocation());
    if (dist < minDist &&
        dist < state.remainingFuel &&
        (station.isAccessibleAtNight() || !isTimeInNightPeriod(state.currentTime + dist) || station == map[m_dataset.targetStation]) &&
        std::find(pathStations.begin(), pathStations.end(), station) == pathStations.end() &&
        std::find(closedStations.begin(), closedStations.end(), station) == closedStations.end()
    ) {
      minDist = dist;
      nearestStation = &station;
    }
  }

  return nearestStation;
}

ProblemPath NaturalBreitlingSolver::solveForPath(const ProblemMap &map, bool *stopFlag /*ignored*/)
{
  const Location destinationLocation = map[m_dataset.targetStation].getLocation();
  const std::vector<PathTarget> targets = generateTargets(map);

  ResolutionState state;
  std::vector<ProblemStation> &pathStations = state.path;
  state.remainingFuel = m_planeCapacity;
  state.currentTime = m_dataset.departureTime;
  state.closedStations.resize(2);
  state.closedStations[0].push_back(map[m_dataset.targetStation]);
  state.closedStations[1].push_back(map[m_dataset.targetStation]);
  pathStations.push_back(map[m_dataset.departureStation]);

  Location currentLocation = map[m_dataset.departureStation].getLocation();

  while(currentLocation != destinationLocation) {
    const PathTarget &nextTarget = targets[state.targetIdx];
    const Location expectedTarget = geometry::interpolateLocations(currentLocation, nextTarget.location, 1.f/std::max(1ull, (unsigned long long) nextTarget.expectedStepsToReach - state.path.size()));
    const ProblemStation *nearest = nearestAccessible(map, state, expectedTarget);

    if (nearest == nullptr) {
      if (pathStations.size() <= 1)
        throw std::runtime_error("No possible path with given inputs"); // exhausted all possible paths

      // backtrack by one station
      state.closedStations[pathStations.size()-1].push_back(pathStations.back());
      pathStations.pop_back();
      currentLocation = pathStations.back().getLocation();
      if (state.targetIdx > 0 && !doesPathCoverTarget(state.path, targets[state.targetIdx - 1]))
        state.targetIdx--; // the path no longers goes through the target of the discarded station

    } else {
      // advance to the next station
      disttime_t distanceToNext = getTimeDistance(currentLocation, nearest->getLocation());
      state.remainingFuel -= distanceToNext;
      state.currentTime += distanceToNext;
      currentLocation = nearest->getLocation();
      pathStations.push_back(*nearest);

      if (getTimeDistance(currentLocation, nextTarget.location) < nextTarget.radius)
        state.targetIdx++; // switch to next target
      if (nearest->canBeUsedToFuel())
        state.remainingFuel = m_planeCapacity; // refuel, does not account for refueling time

      if (state.closedStations.size() <= pathStations.size())
        state.closedStations.emplace_back();
      else
        state.closedStations[pathStations.size()].clear();
      if (pathStations.size() < breitling_constraints::MINIMUM_STATION_COUNT - 1)
        state.closedStations[pathStations.size()].push_back(map[m_dataset.targetStation]);
    }
  }

  return state.path;
}