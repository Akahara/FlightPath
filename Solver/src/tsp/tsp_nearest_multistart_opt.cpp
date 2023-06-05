#include "tsp_nearest_multistart_opt.h"

/*
 * Compute a path in the map passing through all the stations using the nearest neighbour algorithm and an optional optimization algorithm.
 * If a start/end station has been provided, it must be in the map.
 *
 * It uses a multi-start meta-heuristic to improve the result.
 * The returned path is the best path found.
 *
 * This method is multi-threaded and will use the number of threads specified in the constructor.
 */
[[nodiscard]]
ProblemPath TspNearestMultistartOptSolver::solveForPath(const ProblemMap &map, bool *stopFlag) {
    // Variables
    std::vector<std::thread> threads;
    std::mutex mutex;
    ProblemPath bestPath;
    nauticmiles_t bestLength = std::numeric_limits<nauticmiles_t>::max();

    // duplication of the map
    std::vector<const ProblemStation *> leftStations;
    for (const ProblemStation &station : map) {
        leftStations.push_back(&station);
    }

    // Run threads
    for (unsigned int i = 0; i < m_nbThread; i++) {
        std::thread thread{[&]() {
            solveMultiStartThread(map, leftStations, bestPath, bestLength, mutex, stopFlag);
        }};
        threads.push_back(std::move(thread));
    }

    // Wait for threads to finish
    for (auto &thread : threads) {
        thread.join();
    }

    // Return best path
    return bestPath;
}

/*
 * Compute a path in the map passing through all the stations using the nearest neighbour algorithm and an optional optimization algorithm.
 * If a start/end station has been provided, it must be in the map.
 *
 * This method is used by the different threads.
 */
void TspNearestMultistartOptSolver::solveMultiStartThread(const ProblemMap &map, std::vector<const ProblemStation *> &leftStations,
                                                          ProblemPath &bestPath, nauticmiles_t &bestLength, std::mutex &mutex, bool *stopFlag) const {
    const ProblemStation *current_station;

    while (stopFlag == nullptr || !*stopFlag) {
        // Get the current station
        mutex.lock();
        if (!leftStations.empty()) {
            current_station = leftStations.back();
        } else {
            mutex.unlock();
            break;
        }
        leftStations.pop_back();
        mutex.unlock();

        // Compute the distance matrix
        std::vector<std::vector<nauticmiles_t>> distances;
        distances = getDistancesMatrix(map);

        if (m_startStation != nullptr && m_endStation != nullptr) { // Start and end stations are defined (case 4)
            // Modify the distance matrix to set the distance between start and end to 0
            const int startIdx = (std::find(map.begin(), map.end(), *m_startStation) - map.begin());
            const int endIdx = (std::find(map.begin(), map.end(), *m_endStation) - map.begin());
            distances[startIdx][endIdx] = 0;
            distances[endIdx][startIdx] = 0;
        }

        // Compute the path
        ProblemPath path = nearestNeighborPath(map, current_station, &distances);

        // Close the path
        path.push_back(path.front());

        // Optimize the path
        if (m_optAlgo == 0) {
            // Skip optimization
        } else if (m_optAlgo == 2) {
            path = tsp_optimization::o2opt(path, map, &distances, stopFlag);
        } else if (m_optAlgo == 3) {
            path = tsp_optimization::o3opt(path, map, &distances, stopFlag);
        }

        // Start and end stations are not defined and the path is not a cycle (case 1)
        if (m_startStation == nullptr && !m_loop) {
            int max = 0, current = 0, idx = 0;
            for (int i = 0; i < path.size() - 2; i++) { // First to second last station
                current = geometry::distance(path[i].getLocation(), path[i + 1].getLocation());
                if (current > max) {
                    max = current;
                    idx = i;
                }
            }
            path.pop_back();
            std::rotate(path.begin(), path.begin() + idx + 1, path.end());
        }

        // Start station is defined but not the end station and the path is a cycle (case 5)
        if (m_startStation != nullptr && m_loop) {
            path.pop_back();
            auto it = std::find(path.begin(), path.end(), *m_startStation); // Find the start station
            std::rotate(path.begin(), it, path.end());
            path.push_back(path.front());
        }

        // Start station is defined but not the end station and the path is not a cycle (case 3)
        if (m_startStation != nullptr && m_endStation == nullptr && !m_loop) {
            path.pop_back();
            auto it = std::find(path.begin(), path.end(), *m_startStation); // Find the start station
            assert(it != path.end());
            std::rotate(path.begin(), it, path.end());

            int left_dist = geometry::distance(path.front().getLocation(), path.back().getLocation());
            int right_dist = geometry::distance(path.front().getLocation(), path[1].getLocation());
            if (right_dist > left_dist) {
                std::reverse(path.begin(), path.end());
                std::rotate(path.begin(), path.end(), path.end());
            }
        }

        // Start and end stations are defined (case 4)
        if (m_startStation != nullptr && m_endStation != nullptr && !m_loop) {

            // Find the start station and the end station
            const int startIdx = (std::find(path.begin(), path.end(), *m_startStation) - path.begin());
            const int endIdx = (std::find(path.begin(), path.end(), *m_endStation) - path.begin());

            path.pop_back();
            auto it = std::find(path.begin(), path.end(), *m_startStation); // Find the start station
            std::rotate(path.begin(), it, path.end());

            if (path[1] == *m_endStation) {
                std::reverse(path.begin(), path.end());
                std::rotate(path.begin(), path.end() - 1, path.end());
            }
        }

        // Update the best path
        nauticmiles_t currentLength = getLength(path);
        mutex.lock();
        if (currentLength < bestLength) {
            bestLength = currentLength;
            bestPath = path;
        }
        mutex.unlock();
    }
}

/*
 * Compute a path in the map passing through all the stations using the nearest neighbour algorithm.
 * A valid starting station must be provided.
 * To obtain a closed path, the same station should be provided as both the start and end station.
 *
 * THROWS : - invalid_argument exception if start/endStation is null
 *          - invalid_argument exception if start/endStation is not in the map
 */
[[nodiscard]]
ProblemPath TspNearestMultistartOptSolver::nearestNeighborPath(const ProblemMap &map, const ProblemStation * const startStation,
                                                               std::vector<std::vector<nauticmiles_t>> *distances) const {
    std::vector<std::vector<nauticmiles_t>> distanceMatrix;
    if (distances == nullptr) {
        distanceMatrix = getDistancesMatrix(map);
        distances = &distanceMatrix;
    }

    // Check arguments
    if (startStation == nullptr) {
        throw std::invalid_argument("startStation must not be null");
    }

    // Initialize remaining stations
    std::vector<const ProblemStation *> remainingStations;
    for (const ProblemStation &station : map) {
        remainingStations.push_back(&station);
    }

    // Find start station
    const auto startIndex = std::find(remainingStations.begin(), remainingStations.end(), startStation);
    if (startIndex == remainingStations.end()) { // Not found
        throw std::invalid_argument("startStation is not in the map");
    }

    // Remove start station from remaining stations
    remainingStations.erase(startIndex);

    ProblemPath path;

    // Add start station to path
    path.push_back(*startStation);

    // Add remaining stations to path
    while (!remainingStations.empty()) {

        // Find the nearest station
        const ProblemStation *nearestStation = nullptr;
        nauticmiles_t minDistance = std::numeric_limits<nauticmiles_t>::infinity();

        for (const ProblemStation* const station : remainingStations) {

            // Find the distance between the last station in path and the current station
            const int index1 = std::find(map.begin(), map.end(), path.back()) - map.begin();
            const int index2 = std::find(map.begin(), map.end(), *station) - map.begin();
            const nauticmiles_t distance = (*distances)[index1][index2];

            // Update the nearest station
            if (distance < minDistance) {
                minDistance = distance;
                nearestStation = station;
            }
        }

        // Add the nearest station to path
        path.push_back(*nearestStation);

        // Remove the nearest station from remaining stations
        const auto nearestStationIndex = std::find(remainingStations.begin(), remainingStations.end(), nearestStation);
        remainingStations.erase(nearestStationIndex);
    }

    return path;
}
