#include "tsp_optimization.h"


namespace tsp_optimization {

    /*
     * Optimize a path using the 2-opt algorithm.
     * Obviously, this algorithm will not return the optimal path but will only try to improve the given path.
     *
     * Warning : Even if this algorithm is faster than the 3-opt algorithm, it can still take a long time to compute.
     * You can limit the time by setting a maxDuration in seconds (use 0 to disable the time limit).
     *
     * Thread safe
     */
    Path o2opt(const Path &path, const seconds_t maxDuration) {
        Path optimizedPath = path;

        const auto start_time = std::chrono::steady_clock::now();
        const auto end_time = start_time + std::chrono::seconds(maxDuration);

        bool improved = true;

        while (improved) {
            improved = false;
            for (size_t i = 0; i < (optimizedPath.size() - 1); ++i) {
                for (size_t j = i + 1; j < (optimizedPath.size() - 1); ++j) {

                    // Check if we have exceeded the time limit
                    if ((std::chrono::seconds(maxDuration) != std::chrono::seconds(0)) && (std::chrono::steady_clock::now() > end_time)) {
                        return optimizedPath;
                    }

                    if (geometry::distance(optimizedPath[i].getLocation(), optimizedPath[j].getLocation()) +
                        geometry::distance(optimizedPath[i + 1].getLocation(), optimizedPath[j + 1].getLocation()) <
                        geometry::distance(optimizedPath[i].getLocation(), optimizedPath[i + 1].getLocation()) +
                        geometry::distance(optimizedPath[j].getLocation(), optimizedPath[j + 1].getLocation())) {
                        std::reverse(optimizedPath.getStations().begin() + i + 1,
                                     optimizedPath.getStations().begin() + j + 1);
                        improved = true;
                    }
                }
            }
        }
        return optimizedPath;
    }


    /*
     * Optimize a path using the 3-opt algorithm.
     * Obviously, this algorithm will not return the optimal path but will only try to improve the given path.
     *
     * Warning : This algorithm can take a long time to compute.
     * You can limit the time by setting a max_duration in seconds (use 0 to disable the time limit).
     *
     * Thread safe
     */
    [[maybe_unused]] [[nodiscard]]
    Path o3opt(const Path &path, const seconds_t maxDuration) {
        Path optimizedPath = path;

        const auto start_time = std::chrono::steady_clock::now();
        const auto end_time = start_time + std::chrono::seconds(maxDuration);

        bool improved = true;

        while (improved) {
            improved = false;
            for (size_t i = 0; i < (optimizedPath.size() - 1); ++i) {
                for (size_t j = i + 1; j < (optimizedPath.size() - 1); ++j) {
                    for (size_t k = j + 1; k < (optimizedPath.size() - 1); ++k) {

                        // Check if we have exceeded the time limit
                        if ((std::chrono::seconds(maxDuration) != std::chrono::seconds(0)) && (std::chrono::steady_clock::now() > end_time)) {
                            return optimizedPath;
                        }

                        // Check if the new path is better
                        if (geometry::distance(optimizedPath[i].getLocation(), optimizedPath[j].getLocation()) +
                            geometry::distance(optimizedPath[i + 1].getLocation(), optimizedPath[k].getLocation()) +
                            geometry::distance(optimizedPath[j + 1].getLocation(), optimizedPath[k + 1].getLocation())
                            <
                            geometry::distance(optimizedPath[i].getLocation(), optimizedPath[i + 1].getLocation()) +
                            geometry::distance(optimizedPath[j].getLocation(), optimizedPath[j + 1].getLocation()) +
                            geometry::distance(optimizedPath[k].getLocation(), optimizedPath[k + 1].getLocation())) {

                            // Reverse the path between j and k
                            std::reverse(optimizedPath.getStations().begin() + j + 1,
                                         optimizedPath.getStations().begin() + k + 1);

                            // Reverse the path between i and j
                            std::reverse(optimizedPath.getStations().begin() + i + 1,
                                         optimizedPath.getStations().begin() + j + 1);

                            improved = true;
                        }
                    }
                }
            }
        }
        return optimizedPath;
    }
}