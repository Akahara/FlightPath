#include "tspnearest.h"
#include "../geometry.h"
#include <iostream>

Path TspNearest::solveForPath(const GeoMap &map) {
    Path path;

    path.getStations().push_back(&map.getStations().at(0));

    double minDistance;
    double distance;
    size_t index;

    while (path.getStations().size() != map.getStations().size()) {
        minDistance = 100000000000;
        index = 0;
        for (size_t i = 0; i < map.getStations().size(); i++) {
            bool inPath = false;
            for (size_t j = 0; j < path.size(); j++) {
                if (path[j].getName() == map.getStations().at(i).getName()) {
                    inPath = true;
                    break;
                }
            }
            if (!inPath) {
                distance = geometry::distance(path[path.size() - 1].getLocation(), map.getStations().at(i).getLocation());
                if (distance < minDistance) {
                    minDistance = distance;
                    index = i;
                }
            }
        }
        path.getStations().push_back(&map.getStations().at(index));
    }
    return path;
}