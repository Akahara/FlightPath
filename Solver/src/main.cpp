#include <iostream>
#include <map>

#include "geomap.h"
#include "geometry.h"
#include "userinterface.h"
#include "geoserializer/xlsserializer.h"
#include "geoserializer/csvserializer.h"
#include "tsp/tsp_nearest_multistart_opt.h"

ProblemMap adaptMapToProblemMap(const GeoMap &map)
{
  ProblemMap adapted;

  for (const Station &station : map.getStations())
    adapted.push_back(ProblemStation{ &station, station.getNightVFR() == "oui" || station.getNightVFR()=="pcl", station.getFuel() == "oui"}); // TODO properly fill isAccessibleAtNight and canBeUsedToFuel according to the original station data
  //adapted.push_back(ProblemStation{ &station, station.getNightVFR() == "oui", station.getFuel() == "oui"}); // TODO properly fill isAccessibleAtNight and canBeUsedToFuel according to the original station data

  return adapted;
}

void testWriteMap(const ProblemMap &map, const char *fileName)
{
  std::ofstream file{ fileName };
  Location point = map[rand() % map.size()].getLocation();
  nauticmiles_t maxDist = std::numeric_limits<nauticmiles_t>::lowest();

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
    maxDist = std::max(maxDist, geometry::distance(point, station.getLocation()));
  }

  constexpr double padding = 1;
  file << "<svg viewBox=\""
    << (minLon - padding) << " "
    << (minLat - padding) << " "
    << (maxLon - minLon + 2 * padding) << " "
    << (maxLat - minLat + 2 * padding)
    << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";

  for (const ProblemStation &station : map) {
    if(station.isAccessibleAtNight())
      file << "<circle cx=\"" << station.getLocation().lon << "\" cy=\"" << station.getLocation().lat << "\" r=\".2\" fill=\"green\"/>";
  }

  file << "</svg>" << std::endl;
}

/*
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
    breitlingData.planeFuelCapacity = 190; // 190L
    breitlingData.planeSpeed = geography::km2nauticmiles(222); // 222km/h
    breitlingData.planeFuelUsage = 38; // 38L/h
    breitlingData.timeToRefuel = 0;
    //breitlingData.targetStation = BreitlingData::NO_TARGET_STATION;

    //std::unique_ptr<GeoSerializer> serializer = std::make_unique<XLSSerializer>();
    std::unique_ptr<GeoSerializer> serializer = std::make_unique<CSVSerializer>();
    std::unique_ptr<PathSolver> solver = std::make_unique<LabelSettingBreitlingSolver>(breitlingData);
    //std::unique_ptr<PathSolver> solver = std::make_unique<NaturalBreitlingSolver>(breitlingData);

    try {
      //map = serializer->parseMap("Spreadsheet.xlsx");
      map = serializer->parseMap("aerodromes.csv");
    } catch (const std::runtime_error &err) {
      std::cerr << err.what() << std::endl;
      std::cin.get();
      return 1;
    }

    // keep half the stations
    //map.getStations().erase(
    //  std::remove_if(map.getStations().begin(), map.getStations().end(), [](auto&) { return rand() & 1; }),
    //  map.getStations().end());
    problemMap = adaptMapToProblemMap(map);
    testWriteMap(problemMap, "test.svg");

    std::cout << "Map loaded with " << map.getStations().size() << " stations" << std::endl;

    //path = solver->solveForPath(problemMap);
    std::cout << "fuel? " << breitling_constraints::satisfiesFuelConstraints(breitlingData, path) << std::endl;
    std::cout << "path? " << breitling_constraints::satisfiesPathConstraints(map, breitlingData, path) << std::endl;
    std::cout << "regions? " << breitling_constraints::satisfiesRegionsConstraints(path) << std::endl;
    std::cout << "stations? " << breitling_constraints::satisfiesStationCountConstraints(path) << std::endl;
    std::cout << "time? " << breitling_constraints::satisfiesTimeConstraints(breitlingData, path) << std::endl;

    std::cout << "Found path of size " << path.size() << " and length " << path.length() << std::endl;

    interface_mock::writePathToFile(problemMap, path, "out.svg");

    { // temp
      std::unique_ptr<PathSolver> solver = std::make_unique<NaturalBreitlingSolver>(breitlingData);
      path = solver->solveForPath(problemMap);
      std::cout << "With natural breitling: found path of size " << path.size() << " and length " << path.length() << std::endl;
      interface_mock::writePathToFile(problemMap, path, "out_natural.svg");
    }

    std::ofstream kmlFile{ "out.kml" };
    kml_export::writeHeader(kmlFile);
    kml_export::writeAllStationsLayer(kmlFile, map);
    kml_export::writeProblemStationsLayer(kmlFile, problemMap);
    kml_export::writePathLayer(kmlFile, path, "Chemin");
    kml_export::writeFooter(kmlFile);

    std::cin.get();

    return 0;
}
 */

int main() {
    GeoMap map;
    ProblemMap problemMap;
    XLSSerializer serializer;
    ProblemPath path;

    map = serializer.parseMap("aerodromes.xlsx");

    //map.getStations().erase(map.getStations().begin() + 50, map.getStations().end());
    std::cout << "Number of stations: " << map.getStations().size() << std::endl;

    problemMap = adaptMapToProblemMap(map);

    unsigned int nb_threads = 6;
    unsigned int opt_algo = 3;
    //const ProblemStation *start = &problemMap[0];
    //const ProblemStation *end = &problemMap[2];
    //const ProblemStation *end = nullptr;
    bool stop = false;
    bool boucle = false;

    TspNearestMultistartOptSolver solver(nb_threads, opt_algo, boucle, &stop, nullptr, nullptr);


    auto start_time = std::chrono::high_resolution_clock::now();
    path = solver.solveForPath(problemMap);
    auto end_time = std::chrono::high_resolution_clock::now();


    std::cout << "Found path of length " << getLength(path) << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() << "ms" << std::endl;

    interface_mock::writePathToFile(problemMap, path, "out.svg");

    std::ofstream kmlFile{ "out.kml" };
    kml_export::writeHeader(kmlFile);
    kml_export::writeAllStationsLayer(kmlFile, map);
    kml_export::writeProblemStationsLayer(kmlFile, problemMap);
    kml_export::writePathLayer(kmlFile, path, "Chemin");
    kml_export::writeFooter(kmlFile);

    return 0;
}