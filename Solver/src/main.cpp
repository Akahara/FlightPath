#include <iostream>

#include "geomap.h"
#include "geometry.h"
#include "tsp/genetictsp.h"

int main()
{
    GeoMap map;

    size_t T = 10;
    for (size_t x = 0; x < T; x++)
        for (size_t y = 0; y < T; y++)
            map.getStations().push_back(Station{ Location{(double)x,(double)y} });

    std::unique_ptr<PathSolver> solver = std::make_unique<GeneticTSPSolver>();
    Path path = solver->solveForPath(map);

    for (size_t i = 0; i < path.size(); i++) {
        std::cout << i << "\t" << path[i].getLocation().lon << " " << path[i].getLocation().lat << std::endl;
    }

    double l = 0;
    for (size_t i = 0; i < path.size(); i++) {
        Location l1 = path[i].getLocation();
        Location l2 = path[(i+1)%path.size()].getLocation();
        l += std::sqrt((l1.lon - l2.lon) * (l1.lon - l2.lon) + (l1.lat - l2.lat) * (l1.lat - l2.lat));
    }
    std::cout << "total length " << l << " (optimal="<<T*T<<")" << std::endl;

    for (size_t y = 0; y < T; y++) {
        for (size_t x = 0; x < T; x++) {
            std::cout << (int)(std::find_if(path.getStations().begin(), path.getStations().end(), [x, y](const auto *s) { return s->getLocation().lon == x && s->getLocation().lat == y; }) - path.getStations().begin()) << "\t";
        }
        std::cout << "\n\n";
    }
    std::cout.flush();

    std::cout << "<svg viewBox=\"0 0 " << T << " " << T << "\" xmlns=\"http://www.w3.org/2000/svg\">";
    for (size_t i = 0; i < path.size(); i++) {
        Location l1 = path[i].getLocation();
        Location l2 = path[(i + 1) % path.size()].getLocation();
        std::cout << "<line x1=\"" << l1.lon << "\" y1=\"" << l1.lat << "\" x2=\"" << l2.lon << "\" y2=\"" << l2.lat << "\"/>";
    }
    std::cout << "<style>line{stroke-width:.1;stroke:black;}</style>";
    std::cout << "</svg>" << std::endl;

    std::cin.get();
    return 0;
}