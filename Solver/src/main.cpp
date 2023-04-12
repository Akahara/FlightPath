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

    TspNearest solver;

    auto t1 = std::chrono::high_resolution_clock::now();
    Path path = solver.solveForPath(map);
    auto t2 = std::chrono::high_resolution_clock::now();


    std::cout << "Solver time: " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "ms" << std::endl;

    interface_mock::writePathToFile(map, path, "out.svg");

    return 0;
}