#pragma once

#include "../path.h"
#include "../geometry.h"

namespace tsp_optimization {

    /*
     * Optimize a path using the 2-opt algorithm.
     * Obviously, this algorithm will not return the optimal path but will only try to improve the given path.
     *
     * WARNING : Even if this algorithm is faster than the 3-opt algorithm, it can still take a long time to compute on large instance.
     * You can limit the time by setting a timeout in milliseconds (use 0 to disable the time limit).
     *
     * Thread safe
     */
    [[maybe_unused]] [[nodiscard]]
    Path o2opt(const Path &path, const std::chrono::milliseconds timeout);

    /*
     * Optimize a path using the 3-opt algorithm.
     * Obviously, this algorithm will not return the optimal path but will only try to improve the given path.
     *
     * WARNING : This algorithm can take a long time to compute.
     * You can limit the time by setting a timeout in milliseconds (use 0 to disable the time limit).
     *
     * Thread safe
     */
    [[maybe_unused]] [[nodiscard]]
    Path o3opt(const Path &path, const std::chrono::milliseconds timeout);
};

