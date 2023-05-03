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
    XLSSerializer serializer;
    GeoMap map = serializer.parseMap("aerodromes.xlsx");

    // remove all stations after the first
    map.getStations().erase(map.getStations().begin() + 50, map.getStations().end());
    std::cout << "Number of stations: " << map.getStations().size() << std::endl;

    // Create a path
    TspNearest solver;

    Path path;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; ++i) {
        path = solver.solveClosedPath(map, &map.getStations()[0], std::chrono::milliseconds(0), 7);
    }

    auto end = std::chrono::high_resolution_clock::now();

    unsigned int moyenne = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 100;

    std::cout << "Time taken: " << moyenne << " ms" << std::endl;
    std::cout << "Path length: " << path.length() << " nm" << std::endl;

    interface_mock::writePathToFile(map, path, "out.svg");

    return 0;
}