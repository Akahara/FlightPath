#pragma once

#include <thread>
#include <mutex>
#include <assert.h>

#include "../pathsolver.h"
#include "tsp_optimization.h"

class TspNearestMultistartOptSolver : public PathSolver {
private:
    unsigned int m_nbThread;
    unsigned int m_optAlgo;
    bool m_loop;
    bool *m_stop;
    const ProblemStation *m_startStation;
    const ProblemStation *m_endStation;

public:
    /*
     * Constructor of the TspNearestMultistartOptSolver Solver
     * These are the only valid parameter sets (start, loop, end):
     *
     *                          start
     *                           /\
     *                      null/  \!null
     *                     /             \
     *                 loop               loop
     *                  /\                 /\
     *            false/  \true      false/  \true
     *                              /
     *                            end
     *                            /\
     *                       null/  \!null
     *
     * THROWS : - invalid_argument exception if the parameters are invalid
     *          - invalid_argument exception if the number of threads is 0
     *          - invalid_argument exception if the optimization algorithm is invalid
     */
    TspNearestMultistartOptSolver(unsigned int nbThread, unsigned int optAlgo, bool loop, bool *stop, const ProblemStation *startStation,
                                  const ProblemStation *endStation)
      : m_nbThread(nbThread), m_optAlgo(optAlgo), m_loop(loop), m_stop(stop), m_startStation(startStation), m_endStation(endStation)
    {
        // Check arguments
        if (nbThread == 0) {
            throw std::invalid_argument("The number of threads must be greater than 0");
        }
        if (optAlgo != 0 && optAlgo != 2 && optAlgo != 3) {
            throw std::invalid_argument("Invalid optimization algorithm (must be 0, 2 or 3)");
        }
        if (startStation == nullptr && endStation != nullptr) {
            throw std::invalid_argument("The start station must be defined if the end station is defined");
        }
        if (endStation != nullptr && loop) {
            throw std::invalid_argument("The end station must be null if the path is a loop");
        }
    }

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
    ProblemPath solveForPath(const ProblemMap &map);

private:
    /*
     * Compute a path in the map passing through all the stations using the nearest neighbour algorithm and an optional optimization algorithm.
     * If a start/end station has been provided, it must be in the map.
     *
     * This method is used by the different threads.
     */
    void solveMultiStartThread(const ProblemMap &map, std::vector<const ProblemStation *> &leftStations,
                               ProblemPath &bestPath, nauticmiles_t &bestLength, std::mutex &mutex) const;

    /*
     * Compute a path in the map passing through all the stations using the nearest neighbour algorithm.
     * A valid starting station must be provided.
     * To obtain a closed path, the same station should be provided as both the start and end station.
     *
     * THROWS : - invalid_argument exception if start/endStation is null
     *          - invalid_argument exception if start/endStation is not in the map
     */
    [[nodiscard]]
    ProblemPath nearestNeighborPath(const ProblemMap &map, const ProblemStation *const startStation,
                                    std::vector<std::vector<nauticmiles_t>> *distances) const;
};
