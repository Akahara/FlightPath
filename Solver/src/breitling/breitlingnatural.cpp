#include "breitlingnatural.h"

#include <list>
#include <stdexcept>
#include <array>
#include <iostream>

#include "../geometry.h"

#include <fstream>
std::ofstream DBG_FILE{ "temp.svg" };

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


std::list<NaturalBreitlingSolver::PathTarget> NaturalBreitlingSolver::generateTargets(const ProblemMap &map)
{
  constexpr size_t regionCount = breitling_constraints::MANDATORY_REGION_COUNT;

  std::vector<region_t> stationsRegions(map.size(), -1);
  std::vector<RegionGravityCenter> regionsCenters(regionCount);
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

  for (auto &target : targets) {
    DBG_FILE << "<circle cx=\"" << target.location.lon << "\" cy=\"" << target.location.lat << "\" r=\"" << target.radius << "\" fill=\"blue\"/>";
  }
  for (const ProblemStation &station : map) {
    // black - all good
    // pink - no fuel  no night
    // blue - no night
    // red - no fuel
    float red = station.canBeUsedToFuel() ? 0 : 1;
    float green = .1f;
    float blue = station.isAccessibleAtNight() ? 0 : 1;
    DBG_FILE << "<circle cx=\"" << station.getLocation().lon << "\" cy=\"" << station.getLocation().lat << "\" r=\".1\" " << "fill=\"rgb(" << red * 255 << "," << green * 255 << "," << blue * 255 << ")\"/>\n";
    if (breitling_constraints::isStationInMandatoryRegion(*station.getOriginalStation(), 0)) DBG_FILE << "<rect x=\"" << station.getLocation().lon - .1 << "\" y=\"" << station.getLocation().lat - .1 << "\" " << "stroke-width=\".05\" stroke=\"blue\" width = \".2\" height=\".2\" fill=\"transparent\" />";
    if (breitling_constraints::isStationInMandatoryRegion(*station.getOriginalStation(), 1)) DBG_FILE << "<rect x=\"" << station.getLocation().lon - .1 << "\" y=\"" << station.getLocation().lat - .1 << "\" " << "stroke-width=\".05\" stroke=\"green\" width = \".2\" height=\".2\" fill=\"transparent\" />";
    if (breitling_constraints::isStationInMandatoryRegion(*station.getOriginalStation(), 2)) DBG_FILE << "<rect x=\"" << station.getLocation().lon - .1 << "\" y=\"" << station.getLocation().lat - .1 << "\" " << "stroke-width=\".05\" stroke=\"red\" width = \".2\" height=\".2\" fill=\"transparent\" />";
    if (breitling_constraints::isStationInMandatoryRegion(*station.getOriginalStation(), 3)) DBG_FILE << "<rect x=\"" << station.getLocation().lon - .1 << "\" y=\"" << station.getLocation().lat - .1 << "\" " << "stroke-width=\".05\" stroke=\"purple\" width = \".2\" height=\".2\" fill=\"transparent\" />";

    for (auto &target : targets) {
      if(getTimeDistance(target.location, station.getLocation()) < target.radius)
        DBG_FILE << "<rect x=\"" << station.getLocation().lon - .1 << "\" y=\"" << station.getLocation().lat - .1 << "\" " << "stroke-width=\".05\" stroke=\"orange\" width = \".2\" height=\".2\" />";
    }
  }

  return targets;
}

const ProblemStation *NaturalBreitlingSolver::nearestAccessible(const ProblemMap &map, const Path &currentPath, disttime_t remainingFuelCapacity, disttime_t currentTime, Location location)
{
  disttime_t minDist = std::numeric_limits<nauticmiles_t>::max();
  const ProblemStation *nearestStation = nullptr;

  auto &pathStations = currentPath.getStations();

  for (size_t i = 0; i < map.size(); i++) {
    const ProblemStation &station = map[i];
    nauticmiles_t dist = getTimeDistance(location, station.getLocation());
    if (i != m_dataset.targetStation &&
        dist < minDist &&
        dist < remainingFuelCapacity &&
        (station.isAccessibleAtNight() || !isTimeInNightPeriod(currentTime + dist)) &&
        std::find(pathStations.begin(), pathStations.end(), station.getOriginalStation()) == pathStations.end()
    ) {
      minDist = dist;
      nearestStation = &station;
    }
  }

  return nearestStation;
}

Path NaturalBreitlingSolver::solveForPath(const ProblemMap &map)
{
  double
    minLon = std::numeric_limits<double>::max(),
    minLat = std::numeric_limits<double>::max(),
    maxLon = std::numeric_limits<double>::min(),
    maxLat = std::numeric_limits<double>::min();
  for (const ProblemStation &station : map) {
    double lon = station.getLocation().lon;
    double lat = station.getLocation().lat;
    minLon = std::min(minLon, lon);
    minLat = std::min(minLat, lat);
    maxLon = std::max(maxLon, lon);
    maxLat = std::max(maxLat, lat);
  }

  constexpr double padding = 1;
  DBG_FILE << "<svg viewBox=\""
    << (minLon - padding) << " "
    << (minLat - padding) << " "
    << (maxLon - minLon + 2 * padding) << " "
    << (maxLat - minLat + 2 * padding)
    << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";


  constexpr size_t STEPS = breitling_constraints::MINIMUM_STATION_COUNT;

  Path path;

  std::list<PathTarget> nextTargets = generateTargets(map);
  Location currentLocation = map[m_dataset.departureStation].getLocation();
  disttime_t remainingFuelCapacity = m_planeCapacity;
  disttime_t currentTime = m_dataset.departureTime;

  path.getStations().push_back(map[m_dataset.departureStation].getOriginalStation());

  for (size_t step = 0; step < STEPS - 2; step++) {
    const PathTarget &nextTarget = nextTargets.front();
    Location expectedTarget = geometry::interpolateLocations(currentLocation, nextTarget.location, 1.f/std::max(1ull, nextTarget.expectedStepsToReach - step));

    const ProblemStation *nearest = nearestAccessible(map, path, remainingFuelCapacity, currentTime, expectedTarget);
    if (nearest == nullptr)
      throw std::runtime_error("Broken path"); // TODO backtrack

    float h = nextTargets.size() / 4.f;
    DBG_FILE << "<line x1=\"" << currentLocation.lon << "\" y1=\"" << currentLocation.lat << "\" x2=\"" << nearest->getLocation().lon << "\" y2=\"" << nearest->getLocation().lat << "\" stroke=\"rgb(" << h * 0xff << "," << h * 0xff << "," << h * 0xff << ")\" stroke-width=\".05\" />";

    disttime_t distanceToNext = getTimeDistance(currentLocation, nearest->getLocation());
    currentLocation = nearest->getLocation();
    remainingFuelCapacity -= distanceToNext;
    currentTime += distanceToNext;

    if (getTimeDistance(currentLocation, nextTarget.location) < nextTarget.radius)
      nextTargets.pop_front(); // switch to next target
    if (nearest->canBeUsedToFuel())
      remainingFuelCapacity = m_planeCapacity; // refuel

    path.getStations().push_back(nearest->getOriginalStation());

    float w = isTimeInNightPeriod(currentTime) ? .7 : 1;
    DBG_FILE << "<circle cx=\"" << nearest->getLocation().lon << "\" cy=\"" << nearest->getLocation().lat << "\" r=\".08\" " << "fill=\"rgb(" << w * 255 << "," << w * 255 << "," << w * 255 << ")\"/>\n";
  }

  path.getStations().push_back(map[m_dataset.targetStation].getOriginalStation());

  DBG_FILE << "</svg>" << std::endl;
  DBG_FILE.close();
  return path;
}