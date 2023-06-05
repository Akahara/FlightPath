#pragma once

#include "../path.h"
#include "../geometry.h"
#include "../pathsolver.h"

namespace tsp_optimization {

    /*
     * Optimize a path using the 2-opt algorithm.
     * Obviously, this algorithm will not return the optimal path but will only try to improve the given path.
     *
     * You can pass a matrix (vector of vector) of distances between stations.
     * If it is not provided, the distances will be computed using the geometry::distance function.
     *
     * The map is used to know how to read the distances matrix.
     *
     * WARNING : Even if this algorithm is faster than the 3-opt algorithm, it can still take a long time to compute on large instance.
     * You can limit the time by passing a pointer to a boolean that you can set to true to stop the algorithm (in another thread for example).
     */
    [[maybe_unused]] [[nodiscard]]
    ProblemPath o2opt(const ProblemPath &path, const ProblemMap &map, std::vector<std::vector<nauticmiles_t>> *distances = nullptr,
                      bool *stop = nullptr);

    /*
     * Optimize a path using the 3-opt algorithm.
     * Obviously, this algorithm will not return the optimal path but will only try to improve the given path.
     *
     * You can pass a matrix (vector of vector) of distances between stations.
     * If it is not provided, the distances will be computed using the geometry::distance function.
     *
     * The map is used to know how to read the distances matrix.
     *
     * WARNING : This algorithm can take a long time to compute.
     * You can limit the time by passing a pointer to a boolean that you can set to true to stop the algorithm (in another thread for example).
     */
    ProblemPath o3opt(const ProblemPath &path, const ProblemMap &map, std::vector<std::vector<nauticmiles_t>> *distances = nullptr,
                      bool *stop = nullptr);
};

