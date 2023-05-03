#pragma once

#include <iostream>
#include <thread>

#include "../pathsolver.h"
#include "tsp_optimization.h"

class TspNearest : public PathSolver {
public:
    /* ********************************************************************************************* *\
     *                                         NEW FUNCTIONS                                         *
    \* ********************************************************************************************* */
    // TODO(Thomas) :doc
    [[nodiscard]]
    Path solveForPath(const GeoMap &map, const Station* const startStation, const Station* const endStation,
                      std::chrono::milliseconds timeout, unsigned int nbThread) const;

    // TODO(Thomas) :doc
    [[nodiscard]]
    Path solveClosedPath(const GeoMap &map, const Station *const startStation, std::chrono::milliseconds timeout, unsigned int nbThread) const;

    // TODO(Thomas) :doc
    [[nodiscard]]
    Path solveClosedPathThread(const GeoMap &map, std::chrono::milliseconds timeout, unsigned int nbThread, unsigned int threadIdx,
                               unsigned int opt_algo, Path &bestPath, nauticmiles_t &bestLength, std::mutex &mutex) const;

    /*
     * Compute a path in the map passing through all the stations using the nearest neighbour algorithm.
     * A valid starting station must be provided.
     * To obtain a closed path, the same station should be provided as both the start and end station.
     *
     * THROWS : - invalid_argument exception if start/endStation is null
     *          - invalid_argument exception if start/endStation is not in the map
     *
     * Thread safe
     */
    [[nodiscard]]
    Path nearestNeighborPath(const GeoMap &map, const Station* const startStation, const Station* const endStation) const;
};
