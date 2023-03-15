#pragma once

#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <assert.h>
#include <new>
#include <stdexcept>
#include <vector>
#include <list>
#include <array>

#include "BreitlingSolver.h"
#include "../geometry.h"

typedef uint32_t fragmentidx_t;
typedef uint8_t stationidx_t;
typedef float time_t;
typedef time_t distance_t;
typedef uint8_t region_t; // bit field, 0b1010 means that the 2nd and 4th regions have been visited

namespace utils {

template<typename T>
constexpr unsigned char count1bits(T x)
{
  unsigned char c = 0;
  while (x) {
    c += x & 1;
    x >>= 1;
  }
  return c;
}

template<size_t S>
constexpr std::array<unsigned char, S> createBitCountLookupTable()
{
  std::array<unsigned char, S> lookupTable;
  for (size_t i = 0; i < S; i++)
    lookupTable[i] = count1bits(i);
  return lookupTable;
}

static constexpr std::array BIT_COUNT_LOOKUP_TABLE = createBitCountLookupTable<1 << (breitling_constraints::REGION_COUNT-1)>();

// Returns the number of visited regions in a region bit field
// ie. 0b1010 represents regions 2 & 4 being visited, countRegions(0b1010) = 2
static unsigned char countRegions(region_t regionsBitField)
{
  return BIT_COUNT_LOOKUP_TABLE[regionsBitField];
}

}

struct PathFragment {
private:
  uint16_t packedStationUseCount; // 9 bits for the station (up to 512) and 7 bits for the use count (up to 127 immediate children + 1 immediate use)
  fragmentidx_t previousFragmentIdx;

public:
  PathFragment(stationidx_t station, fragmentidx_t previousFragmentIdx)
    : previousFragmentIdx(previousFragmentIdx)
  {
    assert(station & ~0b1'1111'1111 == 0);
    packedStationUseCount = station | (1 << 9); // initialized with 1 use
  }

  inline stationidx_t getStationIdx() const { return packedStationUseCount & 0b1'1111'1111; }
  inline uint8_t getUseCount() const { return packedStationUseCount >> 9; }
  inline void setUseCount(uint8_t count) { packedStationUseCount = getStationIdx() | count << 9; }
  inline fragmentidx_t getPreviousFragment() const { return previousFragmentIdx; }
};

struct Label {
  fragmentidx_t lastFragmentIdx;
  uint8_t visitedRegions;
  uint8_t visitedStationCount;
  float currentTime;
  float currentFuel;
};

class FragmentsArena {
private:
  PathFragment *m_array;
  size_t m_size;
  size_t m_next;

public:
  explicit FragmentsArena(size_t size)
    : m_size(size), m_next(0)
  {
    size_t capacity = sizeof(FragmentsArena) * size;
    m_array = (PathFragment*)malloc(capacity);
    assert(m_array);
    memset(m_array, 0xff, capacity);
  }

  ~FragmentsArena()
  {
    free(m_array);
  }

  FragmentsArena(const FragmentsArena &) = delete;
  FragmentsArena &operator=(const FragmentsArena &) = delete;

  PathFragment &operator[](fragmentidx_t idx) { assert(idx < m_size); return m_array[idx]; }

  fragmentidx_t emplace(fragmentidx_t parent, stationidx_t station)
  {
    for (size_t c = m_size; c > 0; c--) {
      if (isSlotFree((m_next++) % m_size)) {
        fragmentidx_t i = (m_next - 1) % m_size;
        new (&m_array[i]) PathFragment(station, parent); // allocate in place (= only call the constructor)
        return i;
      }
    }
    size_t newSize = (size_t)(m_size *= 1.5f);
    PathFragment *newArray = (PathFragment *)realloc(m_array, newSize);
    if (!newArray) throw std::runtime_error("Out of memory");
    m_array = newArray;
    new (&m_array[m_size]) PathFragment(station, parent);
    m_next = m_size;
    m_size = newSize;
    return m_next++;
  }

  void release(fragmentidx_t fragmentIdx)
  {
    assert(!isSlotFree(fragmentIdx));
    PathFragment &fragment = m_array[fragmentIdx];
    size_t newUseCount = fragment.getUseCount() - 1ull;
    if (newUseCount != 0) {
      fragment.setUseCount(newUseCount);
    } else {
      if (fragment.getStationIdx() != 0)
        release(fragment.getPreviousFragment());
      memset(&fragment, 0xff, sizeof(PathFragment));
    }
  }

private:
  inline bool isSlotFree(fragmentidx_t slot)
  {
    return *(unsigned char*)&m_array[slot] == 0xff;
  }
};

struct LimitedAdjency {
  distance_t distance;
  stationidx_t station;
};

class PartialAdjencyMatrix {
private:
  std::vector<std::vector<LimitedAdjency>> m_adjencyMatrix; // NxM matrix M<<N
  std::vector<distance_t> m_distanceToTarget; // 1xN matrix, the distances to the target station must be kept somehow

public:
  inline size_t adjencyCount(stationidx_t station)
  {
    return m_adjencyMatrix[station].size();
  }

  inline stationidx_t getAdjencyNextStation(stationidx_t currentStation, size_t idx)
  {
    return m_adjencyMatrix[currentStation][idx].station;
  }

  inline stationidx_t getAdjencyNextDistance(stationidx_t currentStation, size_t idx)
  {
    return m_adjencyMatrix[currentStation][idx].distance;
  }

  inline distance_t distanceToTargetStation(stationidx_t fromStation)
  {
    return m_distanceToTarget[fromStation];
  }
};

class FullAdjencyMatrix {
private:
  std::vector<distance_t> m_adjencyMatrix; // unrolled NxN matrix
  size_t                  m_nodeCount;
  const BreitlingData    *m_breitlingData;
  // TODO could be stored as a triangle "matrix" (only half of it) because the NxN matrix is symetric, todo is to check performances

public:
  FullAdjencyMatrix(const GeoMap &geomap, const BreitlingData *breitlingData)
    : m_nodeCount(geomap.getStations().size()), m_breitlingData(&breitlingData)
  {
    m_adjencyMatrix.resize(m_nodeCount * m_nodeCount);
    for (size_t i = 0; i < m_nodeCount; i++) {
      for (size_t j = i; j < m_nodeCount; j++) {
        nauticmiles_t realDistance = geometry::distance(geomap.getStations()[i].getLocation(), geomap.getStations()[j].getLocation());
        time_t timeDistance = realDistance / 4; // TODO
        m_adjencyMatrix[i + j * m_nodeCount] = m_adjencyMatrix[i * m_nodeCount + j] = timeDistance;
      }
    }
  }

  inline size_t adjencyCount(stationidx_t station)
  {
    return m_adjencyMatrix.size();
  }

  inline stationidx_t getAdjencyNextStation(stationidx_t currentStation, size_t idx)
  {
    return idx;
  }

  inline stationidx_t getAdjencyNextDistance(stationidx_t currentStation, size_t idx)
  {
    return m_adjencyMatrix[currentStation][idx];
  }

  inline distance_t distanceToTargetStation(stationidx_t fromStation)
  {
    return m_adjencyMatrix[m_adjencyMatrix.size()-1][fromStation];
  }
};

#if 1
typedef FullAdjencyMatrix AdjencyMatrix;
#else
typedef PartialAdjencyMatrix AdjencyMatrix;
#endif

class LabelSetting {
private:
  static constexpr size_t STATION_TO_VISIT_COUNT = breitling_constraints::MINIMUM_STATION_COUNT;

  //size_t                m_stationCount;
  FragmentsArena        m_fragments = FragmentsArena(100'000); // start with min. 100k fragments, it will surely grow during execution
  AdjencyMatrix         m_adjencyMatrix;
  std::vector<region_t> m_stationRegions;
  const GeoMap         *m_geomap;

public:
  LabelSetting(const GeoMap &geomap, const BreitlingData &breitlingData)
    : m_geomap(&geomap), m_adjencyMatrix(geomap, &breitlingData)
  {
    size_t stationCount = geomap.getStations().size();

    m_stationRegions.resize(stationCount);
    for (size_t i = 0; i < stationCount; i++) {
      for (region_t r = 0; r < breitling_constraints::REGION_COUNT; r++) {
        if (breitling_constraints::isStationInMandatoryRegion(geomap.getStations()[i], r)) {
          m_stationRegions[i] = r;
          break;
        }
      }
    }
  }

private:
  time_t lowerBound(const Label &label)
  {
    return 0; // TODO implement
  }

  void explore(const Label &source, time_t currentBestTime, std::vector<Label> &explorationLabels)
  {
    PathFragment &currentFragment = m_fragments[source.lastFragmentIdx];
    stationidx_t currentStation = currentFragment.getStationIdx();
    for (size_t i = 0; i < m_adjencyMatrix.adjencyCount(currentStation); i++) {
      stationidx_t nextStationIdx = m_adjencyMatrix.getAdjencyNextStation(currentStation, i);
      //const Station &nextStation = m_geomap->getStations()[nextStationIdx];
      distance_t distanceToNext = m_adjencyMatrix.getAdjencyNextDistance(currentStation, i);
      region_t newLabelVisitedRegions = source.visitedRegions | m_stationRegions[nextStationIdx];

      if (distanceToNext > source.currentFuel)
        continue; // not enough fuel
      if (STATION_TO_VISIT_COUNT - source.visitedStationCount > utils::countRegions(newLabelVisitedRegions))
        continue; // 3 regions left to visit but only 2 more stations to go through

      { // without refuel
        Label &explorationLabel = explorationLabels.emplace_back(source); // copy the source
        explorationLabel.currentFuel -= distanceToNext;
        explorationLabel.currentTime += distanceToNext;
        explorationLabel.visitedRegions = newLabelVisitedRegions;
        explorationLabel.visitedStationCount++;
        explorationLabel.lastFragmentIdx = m_fragments.emplace(source.lastFragmentIdx, nextStationIdx);
      }
    }
  }

  bool dominates(Label &dominating, Label &dominated)
  {
    // TODO implement
    return false;
  }

  Path reconstitutePath(fragmentidx_t lastFragment)
  {
    Path path;
    path.getStations().resize(breitling_constraints::MINIMUM_STATION_COUNT);

    // build in reverse
    for (int i = path.size() - 1; i >= 0; i--) {
      path.getStations()[i] = &m_geomap->getStations()[m_fragments[lastFragment].getStationIdx()];
      lastFragment = m_fragments[lastFragment].getPreviousFragment();
    }

    return path;
  }

public:
  Path labelSetting()
  {
    time_t bestTime = std::numeric_limits<time_t>::max();
    fragmentidx_t bestPath = -1;

    std::list<Label> openLabels;
    std::vector<Label> explorationLabels(100);

    while (!openLabels.empty()) {
      Label &explored = openLabels.front();
    
      // the lower bound may have been lowered since the label was added, if
      // it is no longer of intereset discard it before doing any exploration
      if (lowerBound(explored) > bestTime)
        continue;

      // explore the current label to discover new possible *and interesting* paths
      explore(explored, bestTime, explorationLabels);

      if (explorationLabels.size() > 0) {
        for (Label nextLabel : explorationLabels) {
          if (nextLabel.visitedStationCount == breitling_constraints::MINIMUM_STATION_COUNT - 1) {
            // only 1 station remaining, try to complete the path
            distance_t distanceToComplete = m_adjencyMatrix.distanceToTargetStation(m_fragments[nextLabel.lastFragmentIdx].getStationIdx());
            if (nextLabel.currentFuel > distanceToComplete &&
                nextLabel.currentTime + distanceToComplete < bestTime) {
              // path is completeable and better than the best one found so far
              stationidx_t lastStation = m_geomap->getStations().size() - 1;
              if (bestPath != -1)
                m_fragments.release(bestPath);
              bestPath = m_fragments.emplace(nextLabel.lastFragmentIdx, lastStation);
            }
          } else {
            // remove dominated labels
            auto it = openLabels.begin();
            while (it != openLabels.end()) {
              if (dominates(nextLabel, *it)) {
                m_fragments.release(it->lastFragmentIdx);
                it = openLabels.erase(it);
              } else {
                ++it;
              }
            }
            // append the new label to the open list
            openLabels.push_back(nextLabel); // push_back for breadth-first search
                                             // TODO should be a bisect insertion based on a heuristic to explore most interesting labels first
          }
        }
        explorationLabels.clear();
      }

      m_fragments.release(explored.lastFragmentIdx);
      openLabels.pop_front();

      return reconstitutePath(bestPath);
    }
  }
};