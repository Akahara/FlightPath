#include <iostream>
#include <chrono>

#include "geomap.h"
#include "geometry.h"
#include "tsp/genetictsp.h"
#include "userinterface.h"
#include "geoserializer/xlsserializer.h"
#include "tsp/tspnearest.h"

int main()
{
    /*
    std::vector<int> amount_station = { 50, 100, 150 };
    std::vector<int> sum_factor = { 300, 50, 5 };
    std::vector<int> amount_core = { 1, 2, 4, 6, 7, 8 };
    std::vector<int> results = {};

    std::vector<int> amount_station = { 398 };
    std::vector<int> sum_factor = { 1 };
    std::vector<int> amount_core = { 1, 2, 4, 6, 7, 8 };
    std::vector<int> results = {};

    XLSSerializer serializer;
    //GeoMap map = serializer.parseMap("aerodromes.xlsx");

    // Create a path
    TspNearest solver;

    Path path;

    // for each amount of station
    for (int amount : amount_station) {
        // copy map
        GeoMap map = serializer.parseMap("aerodromes.xlsx");
        map.getStations().erase(map.getStations().begin() + amount, map.getStations().end());
        int sumfact = sum_factor.at(std::find(amount_station.begin(), amount_station.end(), amount) - amount_station.begin());
        std::cout << "Number of stations: " << map.getStations().size() << " with a sum factor of " << sumfact << std::endl;
        // for each amount of core
        for (int core : amount_core) {
            std::cout << "Number of core: " << core << std::endl;

            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < sumfact; ++i) {
                path = solver.solveClosedPath(map, &map.getStations()[0], std::chrono::milliseconds(0), core);
            }

            auto end = std::chrono::high_resolution_clock::now();

            unsigned int moyenne = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / sumfact;

            results.push_back(moyenne);

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // transpose results
    std::vector<int> results_transposed = {};
    for (int i = 0; i < amount_core.size(); ++i) {
        for (int j = 0; j < amount_station.size(); ++j) {
            results_transposed.push_back(results.at(i + j * amount_core.size()));
        }
    }

    std::cout << "Results : " << std::endl;
    for (int i = 0; i < results_transposed.size(); ++i) {
        std::cout << results_transposed.at(i) << " ";
        if ((i + 1) % amount_station.size() == 0) {
            std::cout << std::endl;
        }
    }
    */



    XLSSerializer serializer;
    GeoMap map = serializer.parseMap("aerodromes.xlsx");

    TspNearest solver;

    Path path;

    //map.getStations().erase(map.getStations().begin() + 50, map.getStations().end());
    std::cout << "Number of stations: " << map.getStations().size() << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    path = solver.solveClosedPath(map, &map.getStations()[0], std::chrono::milliseconds(0), 6);

    auto end = std::chrono::high_resolution_clock::now();

    unsigned int moyenne = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Time : " << moyenne << std::endl;
    std::cout << "Path length : " << path.length() << std::endl;

    interface_mock::writePathToKML(map, path, "path.kml");

    serializer.writePath("path.xlsx", path);

    return 0;
}