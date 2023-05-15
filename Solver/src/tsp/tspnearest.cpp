#include "tspnearest.h"


[[nodiscard]]
Path TspNearest::solveForPath(const GeoMap &map, const Station *const startStation, const Station *const endStation,
                              std::chrono::milliseconds timeout,  unsigned int nbThread) const {
    auto start = std::chrono::high_resolution_clock::now();

    // Compute nearest path
    Path path = nearestNeighborPath(map, startStation, endStation);

    auto middle = std::chrono::high_resolution_clock::now();
    auto remainingTime = std::chrono::milliseconds(timeout.count() - std::chrono::duration_cast<std::chrono::milliseconds>(middle - start).count());


    std::cout << "Nearest path computed in " << std::chrono::duration_cast<std::chrono::milliseconds>(middle - start).count() << "ms" << std::endl;

    // Optimize path
    return tsp_optimization::o3opt(path, remainingTime);
}

[[nodiscard]]
Path TspNearest::solveClosedPath(const GeoMap &map, const Station *const startStation,
                                 std::chrono::milliseconds timeout, unsigned int nbThread) const {
    std::vector<std::thread> threads;

    // Variables
    Path bestPath;
    nauticmiles_t bestLength = std::numeric_limits<nauticmiles_t>::max();
    bool stop = false;

    // Mutex to protect bestPath and bestLength
    std::mutex mutex;

    // Run threads
    for(unsigned int threadIdx = 0; threadIdx < nbThread; ++threadIdx) {
        threads.emplace_back(&TspNearest::solveClosedPathThread, this, std::ref(map), nbThread, threadIdx, 3, std::ref(bestPath), std::ref(bestLength), std::ref(mutex), &stop);
    }

    // If timeout is not 0, wait for it
    if (timeout != std::chrono::milliseconds(0)) {
        std::this_thread::sleep_for(timeout);
        stop = true;
    }

    // Wait for threads to finish
    for(auto &thread : threads) {
        thread.join();
    }

    return bestPath;
}

[[nodiscard]]
Path TspNearest::solveClosedPathThread(const GeoMap &map, unsigned int nbThread, unsigned int threadIdx,
                                       unsigned int opt_algo, Path &bestPath, nauticmiles_t &bestLength, std::mutex &mutex, bool *stop) const {
    unsigned int nbStations = map.getStations().size();
    unsigned int firstStationIdx = nbStations * threadIdx / nbThread; // First station to compute with this thread
    unsigned int lastStationIdx = nbStations * (threadIdx + 1) / nbThread - 1; // Last station to compute with this thread

    for (unsigned int idx = firstStationIdx ; idx <= lastStationIdx ; ++idx) {
        const Station *const startStation = &map.getStations()[idx];

        // Compute nearest path
        Path path = nearestNeighborPath(map, startStation, startStation);

        // Optimize path
        if (opt_algo == 2) {
            path = tsp_optimization::o2opt(path, stop);
        } else if (opt_algo == 3) {
            path = tsp_optimization::o3opt(path, map, stop);
        } else {
            throw std::invalid_argument("Invalid optimization algorithm (must be 2 or 3)");
        }

        nauticmiles_t length = path.length();
        if (length < bestLength) {
            mutex.lock();
            bestPath = path;
            bestLength = length;
            mutex.unlock();
        }
    }

    return bestPath;
}


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
Path TspNearest::nearestNeighborPath(const GeoMap &map, const Station *const startStation, const Station *const endStation) const {
    // Check arguments
    if (startStation == nullptr) {
        throw std::invalid_argument("startStation must not be null");
    }
    if (endStation == nullptr) {
        throw std::invalid_argument("endStation must not be null");
    }

    // Initialize remaining stations
    std::vector<const Station *> remainingStations;
    for (const Station &station : map.getStations()) {
        remainingStations.push_back(&station);
    }

    // Find start station
    const auto startIndex = std::find(remainingStations.begin(), remainingStations.end(), startStation);
    if (startIndex == remainingStations.end()) { // Not found
        throw std::invalid_argument("startStation is not in the map");
    }

    // Remove start station from remaining stations
    remainingStations.erase(startIndex);

    if (startStation != endStation) {
        // Find end station
        const auto endIndex = std::find(remainingStations.begin(), remainingStations.end(), endStation);
        if (endIndex == remainingStations.end()) { // Not found
            throw std::invalid_argument("endStation is not in the map");
        }

        // Remove end stations from remaining stations
        remainingStations.erase(endIndex); // endStation will be added back at the end
    }

    Path path;

    // Add start station to path
    path.getStations().push_back(startStation);

    // Add remaining stations to path
    while (!remainingStations.empty()) {
        // Find the nearest station
        const Station *nearestStation = nullptr;
        nauticmiles_t minDistance = std::numeric_limits<nauticmiles_t>::infinity();
        for (const Station* const station : remainingStations) {
            const nauticmiles_t distance = geometry::distance(path.getStations().back()->getLocation(), station->getLocation());
            if (distance < minDistance) {
                minDistance = distance;
                nearestStation = station;
            }
        }

        // Add the nearest station to path
        path.getStations().push_back(nearestStation);

        // Remove the nearest station from remaining stations
        const auto nearestStationIndex = std::find(remainingStations.begin(), remainingStations.end(), nearestStation);
        remainingStations.erase(nearestStationIndex);
    }

    // Add end station to path
    path.getStations().push_back(endStation);

    return path;
}

