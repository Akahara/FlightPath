#include "tspnearest.h"
#include "../geometry.h"
#include <iostream>

Path TspNearest::solveForPath(const GeoMap &map) {
    Path path = computeNearestPath(map);
    std:: cout << path.length() << std::endl;

    double oldLength = path.length();
    double newLength = 0;

    std::cout << "3-opt..." << std::endl;

    while (oldLength != newLength) {
        oldLength = path.length();
        o3opt(path);
        newLength = path.length();
        std::cout << newLength << std::endl;
    }

    return path;
}

Path TspNearest::computeNearestPath(const GeoMap &map) {
    std::vector<const Station *> stations;
    for (const Station &station : map.getStations()) {
        stations.push_back(&station);
    }

    Path path;

    path.getStations().push_back(stations[0]);
    stations.erase(stations.begin());

    double minDistance;
    double distance;
    size_t index;

    while (!stations.empty()) {
        minDistance = 999999999999999;
        index = 0;
        for (size_t i = 0; i < stations.size(); i++) {
            distance = geometry::distance(path[path.size() - 1].getLocation(), stations[i]->getLocation());
            if (distance < minDistance) {
                minDistance = distance;
                index = i;
            }
        }
        path.getStations().push_back(stations[index]);
        stations.erase(stations.begin() + index);
    }
    return path;
}

void TspNearest::o2opt(Path &path) {
    for (size_t i = 0; i < path.size() - 1; i++) {
        for (size_t j = i + 1; j < path.size() - 1; j++) {
            if (geometry::distance(path[i].getLocation(), path[j].getLocation()) +
                geometry::distance(path[i + 1].getLocation(), path[j + 1].getLocation()) <
                geometry::distance(path[i].getLocation(), path[i + 1].getLocation()) +
                geometry::distance(path[j].getLocation(), path[j + 1].getLocation())) {
                std::reverse(path.getStations().begin() + i + 1, path.getStations().begin() + j + 1);
            }
        }
    }
}

void TspNearest::o3opt(Path &path) {
    for (size_t i = 0; i < path.size() - 1; i++) {
        for (size_t j = i + 1; j < path.size() - 1; j++) {
            for (size_t k = j + 1; k < path.size() - 1; k++) {
                if (geometry::distance(path[i].getLocation(), path[j].getLocation()) +
                    geometry::distance(path[i + 1].getLocation(), path[k].getLocation()) +
                    geometry::distance(path[j + 1].getLocation(), path[k + 1].getLocation()) <
                    geometry::distance(path[i].getLocation(), path[i + 1].getLocation()) +
                    geometry::distance(path[j].getLocation(), path[j + 1].getLocation()) +
                    geometry::distance(path[k].getLocation(), path[k + 1].getLocation())) {
                    std::reverse(path.getStations().begin() + i + 1, path.getStations().begin() + j + 1);
                    std::reverse(path.getStations().begin() + j + 1, path.getStations().begin() + k + 1);
                }
            }
        }
    }
}
