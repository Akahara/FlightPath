#pragma once

#include "geomap.h"
#include "path.h"

struct ProblemStation {
private:
  const Station *m_station;
  bool m_isAccessibleAtNight;
  bool m_canBeUsedToFuel;
public:
  ProblemStation(const Station *station, bool isAccessibleAtNight, bool canBeUsedToFuel)
    : m_station(station), m_isAccessibleAtNight(isAccessibleAtNight), m_canBeUsedToFuel(canBeUsedToFuel)
  {
  }

  ProblemStation() 
    : m_station(nullptr), m_isAccessibleAtNight(false), m_canBeUsedToFuel(false)
  {
  }

  const Location &getLocation() const { return m_station->getLocation(); }
  bool isAccessibleAtNight() const { return m_isAccessibleAtNight; }
  bool canBeUsedToFuel() const { return m_canBeUsedToFuel; }
  const Station *getOriginalStation() const { return m_station; }
  bool operator==(const ProblemStation &other) const { return m_station == other.m_station; }
  bool operator!=(const ProblemStation &other) const { return m_station != other.m_station; }
  bool operator<(const ProblemStation &other) const { return m_station < other.m_station; } // necessary for std::set<ProblemStation>
};

typedef std::vector<ProblemStation> ProblemMap;
typedef std::vector<ProblemStation> ProblemPath;

inline std::vector<std::vector<nauticmiles_t>> getDistancesMatrix(const ProblemMap &map) {
    std::vector<std::vector<nauticmiles_t>> distances(map.size(), std::vector<nauticmiles_t>(map.size(), 0));

    for (size_t i = 0; i < map.size(); ++i) {
        for (size_t j = 0; j < map.size(); ++j) {
            distances[i][j] = geometry::distance(map[i].getLocation(), map[j].getLocation());
        }
    }

    return distances;
}

// should be namespaced
inline nauticmiles_t getLength(const ProblemPath &path) {
    nauticmiles_t length = 0;
    for (size_t i = 1; i < path.size(); i++)
        length += geometry::distance(path[i].getLocation(), path[i - 1].getLocation());
    return length;
}

class PathSolver {
public:
    /*
     * Solves a problem with the given input map as data.
     * 
     * More often than not the solver will take an indefinite amount of time,
     * to avoid stalling infinetely the second parameter can be used to stop
     * the algorithm, when the flag (is not null and) is set to true the solver
     * should stop immediately.
     * It is to the discretion of the implementation to respect the flag or not,
     * algorithms that end quickly or that cannot be interupted mid-way may choose
     * to ignore the flag completely.
     */
    virtual ProblemPath solveForPath(const ProblemMap &map, bool *stopFlag=nullptr) = 0;
};

