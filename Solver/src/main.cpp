#include <iostream>
#include <map>

#include "geomap.h"
#include "geometry.h"
#include "userinterface.h"
#include "geoserializer/xlsserializer.h"
#include "geoserializer/csvserializer.h"
#include "breitling/breitlingsolver.h"
#include "breitling/label_setting_breitling.h"
#include "breitling/breitlingnatural.h"

ProblemMap adaptMapToProblemMap(const GeoMap &map)
{
  ProblemMap adapted;

  for (const Station &station : map.getStations())
    //adapted.push_back(ProblemStation{ &station, station.getNightVFR() == "oui" || station.getNightVFR()=="pcl", station.getFuel() == "oui"}); // TODO properly fill isAccessibleAtNight and canBeUsedToFuel according to the original station data
  adapted.push_back(ProblemStation{ &station, station.getNightVFR() == "oui", station.getFuel() == "oui"}); // TODO properly fill isAccessibleAtNight and canBeUsedToFuel according to the original station data

  return adapted;
}

void testWriteMapDists(const GeoMap &geomap, const char *fileName)
{
  std::ofstream file{ fileName };
  Location point = geomap.getStations()[rand() % geomap.getStations().size()].getLocation();
  nauticmiles_t maxDist = std::numeric_limits<nauticmiles_t>::lowest();

  double
    minLon = std::numeric_limits<double>::max(),
    minLat = std::numeric_limits<double>::max(),
    maxLon = std::numeric_limits<double>::min(),
    maxLat = std::numeric_limits<double>::min();
  for (const Station &station : geomap.getStations()) {
    double lon = station.getLocation().lon;
    double lat = station.getLocation().lat;
    minLon = std::min(minLon, lon);
    minLat = std::min(minLat, lat);
    maxLon = std::max(maxLon, lon);
    maxLat = std::max(maxLat, lat);
    maxDist = std::max(maxDist, geometry::distance(point, station.getLocation()));
  }

  constexpr double padding = 1;
  file << "<svg viewBox=\""
    << (minLon - padding) << " "
    << (minLat - padding) << " "
    << (maxLon - minLon + 2 * padding) << " "
    << (maxLat - minLat + 2 * padding)
    << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";

  for (const Station &station : geomap.getStations()) {
    float dist = geometry::distance(station.getLocation(), point);
    if (dist == 0)
      file << "<circle cx=\"" << station.getLocation().lon << "\" cy=\"" << station.getLocation().lat << "\" r=\".2\" fill=\"green\"/>";
    else
    file << "<circle cx=\"" << station.getLocation().lon << "\" cy=\"" << station.getLocation().lat << "\" r=\".2\" fill=\"" 
      << "rgb(" << (dist / maxDist)*255 << "," << (dist / maxDist) * 255 << ",1)"
      << "\"/>\n";
  }

  file << "</svg>" << std::endl;
}

int main()
{
    GeoMap map;
    Path path;
    ProblemMap problemMap;

    BreitlingData breitlingData{};
    breitlingData.departureStation = 0;
    breitlingData.targetStation = 1;
    breitlingData.departureTime = 8;
    breitlingData.nauticalDaytime = 8;
    breitlingData.nauticalNighttime = 20;
    breitlingData.planeFuelCapacity = 1000;
    breitlingData.planeSpeed = 100;
    breitlingData.planeFuelUsage = .01f;
    breitlingData.timeToRefuel = 0;

    //std::unique_ptr<GeoSerializer> serializer = std::make_unique<XLSSerializer>();
    std::unique_ptr<GeoSerializer> serializer = std::make_unique<CSVSerializer>();
    //std::unique_ptr<PathSolver> solver = std::make_unique<LabelSettingBreitlingSolver>(breitlingData);
    std::unique_ptr<PathSolver> solver = std::make_unique<NaturalBreitlingSolver>(breitlingData);

    try {
      //map = serializer->parseMap("Spreadsheet.xlsx");
      map = serializer->parseMap("aerodromes.csv");
    } catch (const std::runtime_error &err) {
      std::cerr << err.what() << std::endl;
      std::cin.get();
      return 1;
    }

    //testWriteMapDists(map, "test_dist1.svg");
    //testWriteMapDists(map, "test_dist2.svg");
    //testWriteMapDists(map, "test_dist3.svg");
    //testWriteMapDists(map, "test_dist4.svg");

    // keep half the stations
    //map.getStations().erase(
    //  std::remove_if(map.getStations().begin(), map.getStations().end(), [](auto&) { return rand() & 1; }),
    //  map.getStations().end());
    problemMap = adaptMapToProblemMap(map);

    std::cout << "Map loaded with " << map.getStations().size() << " stations" << std::endl;

    path = solver->solveForPath(problemMap);

    std::cout << "Found path of size " << path.size() << " and length " << path.length() << std::endl;

    interface_mock::writePathToFile(problemMap, path, "out.svg");

    std::cin.get();

    return 0;
}