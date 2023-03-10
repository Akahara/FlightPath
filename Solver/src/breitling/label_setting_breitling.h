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

typedef uint32_t fragmentidx_t;
typedef uint8_t stationidx_t;
typedef float time_t;

struct PathFragment {
private:
  uint16_t packedStationUseCount;
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

class LabelSetting {
private:
  FragmentsArena m_fragments = FragmentsArena(100'000); // start with min. 100k fragments, it will surely grow during execution
  size_t stationCount;

private:
  time_t lowerBound(const Label &label)
  {
    return 0; // TODO implement
  }

  void explore(const Label &source, time_t currentBestTime, std::vector<Label> &explorationLabels)
  {
    // TODO implement
    // (fill explorationLabels)
  }

  Label appendStationToPath(Label label, stationidx_t stationIdx)
  {
    // TODO implement
    return label;
  }

  bool dominates(Label &dominating, Label &dominated)
  {
    // TODO implement
    return false;
  }

  Path reconstitutePath(fragmentidx_t lastFragment)
  {
    Station **stations; // TODO have a GeoMap in LabelSetting
    // this variable exists until then, for the rest of the code to build

    Path path;
    path.getStations().resize(breitling_constraints::MINIMUM_STATION_COUNT);

    // build in reverse
    for (int i = path.size() - 1; i >= 0; i--) {
      path.getStations()[i] = stations[m_fragments[lastFragment].getStationIdx()];
      lastFragment = m_fragments[lastFragment].getPreviousFragment();
    }

    return path;
  }

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
            // complete the path and check against the best one found so far
            Label completed = appendStationToPath(nextLabel, stationCount-1);
            if (completed.currentTime < bestTime) {
              bestTime = completed.currentTime;
              if (bestPath != -1) m_fragments.release(bestPath);
              bestPath = completed.lastFragmentIdx;
            }
          } else {
            // remove dominated labels // TODO it is probably not necessary to filter once per new label, doing it a single time is probably enough (todo is check that theory before changing the code)
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