#include "tsp_optimization.h"


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
    ProblemPath o2opt(const ProblemPath &path, const ProblemMap &map, std::vector<std::vector<nauticmiles_t>> *distances,
                      bool *stop) {
        // Handle errors
        if (path.empty()) {
            throw std::invalid_argument("The path cannot be empty");
        }

        // Compute the distance matrix if it is not provided
        std::vector<std::vector<nauticmiles_t>> distanceMatrix;
        if (distances == nullptr) {
            distanceMatrix = getDistancesMatrix(map);
            distances = &distanceMatrix;
        }

        ProblemPath optimizedPath = path;

        // Get the order of the stations in the path
        std::vector<int> pathOrder{};
        for (const ProblemStation &station : optimizedPath) {
            int index = std::find(map.begin(), map.end(), station) - map.begin();
            pathOrder.push_back(index);
        }

        bool improved = true;

        while (improved) {
            improved = false;
            for (size_t i = 0; i < (optimizedPath.size() - 1); ++i) {
                for (size_t j = i + 1; j < (optimizedPath.size() - 1); ++j) {

                    // Check if we have exceeded the time limit
                    if (stop && *stop) {
                        return optimizedPath;
                    }

                    if ((*distances)[pathOrder[i]][pathOrder[j]] +
                        (*distances)[pathOrder[i + 1]][pathOrder[j + 1]]
                        <
                        (*distances)[pathOrder[i]][pathOrder[i + 1]] +
                        (*distances)[pathOrder[j]][pathOrder[j + 1]]) {

                        // Change the order of the stations
                        std::reverse(pathOrder.begin() + i + 1,
                                     pathOrder.begin() + j + 1);

                        improved = true;
                    }
                }
            }
        }

        // Convert path order to path
        optimizedPath.clear();
        for (size_t i = 0; i < pathOrder.size(); ++i) {
            optimizedPath.push_back(map[pathOrder[i]]);
        }

        return optimizedPath;
    }


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
    ProblemPath o3opt(const ProblemPath &path, const ProblemMap &map, std::vector<std::vector<nauticmiles_t>> *distances,
                      bool *stop) {
        // Handle errors
        if (path.empty()) {
            throw std::invalid_argument("The path cannot be empty");
        }

        // Compute the distance matrix if it is not provided
        std::vector<std::vector<nauticmiles_t>> distanceMatrix;
        if (distances == nullptr) {
            distanceMatrix = getDistancesMatrix(map);
            distances = &distanceMatrix;
        }

        ProblemPath optimizedPath = path;

        // Get the order of the stations in the path
        std::vector<int> pathOrder{};
        for (const ProblemStation &station : optimizedPath) {
            int index = std::find(map.begin(), map.end(), station) - map.begin();
            pathOrder.push_back(index);
        }

        bool improved = true;

        while (improved) {
            improved = false;
            for (size_t i = 0 ; i < (optimizedPath.size() - 1); ++i) {
                for (size_t j = i + 1; j < (optimizedPath.size() - 1); ++j) {
                    for (size_t k = j + 1; k < (optimizedPath.size() - 1); ++k) {

                        // Check if we have exceeded the time limit
                        if (stop && *stop) {
                            return optimizedPath;
                        }

                        // Change path if the new one is better
                        if ((*distances)[pathOrder[i]][pathOrder[j]] +
                            (*distances)[pathOrder[i + 1]][pathOrder[k]] +
                            (*distances)[pathOrder[j + 1]][pathOrder[k + 1]]
                            <
                            (*distances)[pathOrder[i]][pathOrder[i + 1]] +
                            (*distances)[pathOrder[j]][pathOrder[j + 1]] +
                            (*distances)[pathOrder[k]][pathOrder[k + 1]]) {

                            // Reverse the path between j and k
                            std::reverse(pathOrder.begin() + j + 1,
                                         pathOrder.begin() + k + 1);

                            // Reverse the path between i and j
                            std::reverse(pathOrder.begin() + i + 1,
                                         pathOrder.begin() + j + 1);

                            improved = true;
                        }
                    }
                }
            }
        }

        // Convert path order to path
        optimizedPath.clear();
        for (size_t i = 0; i < pathOrder.size(); ++i) {
            optimizedPath.push_back(map[pathOrder[i]]);
        }

        return optimizedPath;
    }
}