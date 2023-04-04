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
#include <queue>
#include <bitset>

#include "BreitlingSolver.h"
#include "../geometry.h"

/*
 * Indices, Allocators and Memory
 * 
 * The label setting algorithm is very memory hungry,
 * an approach that is used several times in this implementation
 * is to store the data as efficiently as possible, with little
 * to no overhead. To do so custom memory allocators are defined,
 * they mostly do not support pointer consistency so indices are
 * used to access stations/fragments/labels in buffers instead of
 * pointers.
 */

/*
 * Time/Distance relation
 * 
 * Because the algorithm operates on distances and time equivalently
 * all distances are first converted to time based on the plane speed,
 * that way computations are done once during initialization and time
 * offsets are immediate.
 * You will see time_t and distance_t used interchangeably in code.
 */

/*
 * TODO update comments after having removed path fragments
 * 
 * Adjency matrices
 * 
 * Adjency matrices are the structure that caches distances between
 * stations. The standard (full) adjency matrix has size stationCount x stationCount
 * and covers all pairs, a partial adjency matrix can be used, it
 * has size N x stationCount and only keeps track of the N closest
 * stations for any station. The current implementation also garanties
 * that a station with fuel is accessible (during day time) even if
 * it is not in the N closest.
 * Given our current problem N can be low and the heuristic will
 * still be fine. This also allows us to make assumptions on the
 * number of explored labels per iteration (it must be less than
 * 2N, each station can be visited once with and without fuelling).
 * In our implementation we assume that a label won't have more than
 * 127 children and that assumption in turn allows us to only use 7
 * bits for the 'use count' field of PathFragments.
 * 
 * Note that the full adjency matrix implementation is yet to be realized.
 */

namespace packed_data_structure {

constexpr size_t MAX_SUPPORTED_STATIONS = 512;
constexpr size_t BITS_PER_STATION_IDX = std::bit_width(MAX_SUPPORTED_STATIONS - 1);
constexpr size_t MAX_CONCURENT_LABELS = 4096;
constexpr size_t BITS_PER_LABEL_IDX = std::bit_width(MAX_CONCURENT_LABELS - 1);
constexpr size_t MAX_REGION_COUNT = breitling_constraints::MANDATORY_REGION_COUNT;
constexpr size_t BITS_PER_REGION_SET = MAX_REGION_COUNT;
constexpr size_t BITS_FOR_VISITED_STATION_COUNT = std::bit_width(breitling_constraints::MINIMUM_STATION_COUNT-1);

}

 // TODO organize classes better

typedef uint16_t stationidx_t;  // index in the Geomap
typedef uint32_t labelidx_t;    // index in the LabelsArena
typedef float time_t;           // a duration (unit is defined by the user)
typedef time_t distance_t;      // a distance, see the comment on time/distance relation
typedef uint8_t region_t;       // bit field, 0b1010 means that the 2nd and 4th regions have been visited
typedef std::bitset<packed_data_structure::MAX_SUPPORTED_STATIONS> stationset_t; // TODO comment

// make sure that all regions combinations can be represented with region_t
// TODO change assertions after having optimized the Label struct
static_assert(breitling_constraints::MANDATORY_REGION_COUNT < sizeof(region_t) * CHAR_BIT);

namespace utils {

/*
 * Returns the number of bits set to 1 in x.
 * ie. count1bits(0b1100'1101) = 5
 */
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

/*
 * A bit count lookup table of size S is a table that can be indexed by x<S and the retrieved value
 * is the number of 1 bits in the binary representation of x. Take care to index with unsigned integers.
 */
template<size_t S>
constexpr std::array<unsigned char, S> createBitCountLookupTable()
{
  std::array<unsigned char, S> lookupTable;
  for (size_t i = 0; i < S; i++)
    lookupTable[i] = count1bits(i);
  return lookupTable;
}

static constexpr std::array BIT_COUNT_LOOKUP_TABLE = createBitCountLookupTable<1 << (breitling_constraints::MANDATORY_REGION_COUNT-1)>();

// Returns the number of visited regions in a region bit field
// ie. 0b1010 represents regions 2 & 4 being visited, countRegions(0b1010) = 2
static unsigned char countRegions(region_t regionsBitField)
{
  return BIT_COUNT_LOOKUP_TABLE[regionsBitField];
}

static inline time_t timeDistanceBetweenStations(const ProblemStation &s1, const ProblemStation &s2, const BreitlingData &dataset)
{
  nauticmiles_t realDistance = geometry::distance(s1.getLocation(), s2.getLocation());
  time_t timeDistance = realDistance / dataset.planeSpeed;
  return timeDistance;
}

static inline time_t planeTimeFuelCapacity(const BreitlingData &dataset)
{
  return dataset.planeFuelCapacity / dataset.planeFuelUsage;
}

/*
 * Assumes that 0..nauticalDaytime and nauticalNighttime..24 are night periods,
 * that is, 0 < nauticalDaytime < nauticalNighttime < 24
 */
static inline bool isTimeInNightPeriod(time_t time, const BreitlingData &dataset)
{
  time = fmod(time, 24.f);
  return time < dataset.nauticalDaytime || time > dataset.nauticalNighttime;
}

} // !namespace utils

#ifdef _DEBUG
namespace profiling_stats {

static struct {
  size_t fragmentsReallocCount;
  size_t labelsReallocCount;
  size_t exploredLabels;
} stats;

inline void onFragmentsRealloc() { stats.fragmentsReallocCount++; }
inline void onLabelsRealloc() { stats.labelsReallocCount++; }
inline void onLabelExplored() { stats.exploredLabels++; }

}
#else
namespace profiling_stats {

inline void onFragmentsRealloc() {}
inline void onLabelsRealloc() {}
inline void onLabelExplored() {}

}
#endif

/*
 * Label of the Label Setting algorithm
 * 
 * TODO optimize the struct in size
 * TODO implement the Ir strategy
 */
struct Label {
  stationset_t visitedStations;
  stationidx_t currentStation : packed_data_structure::BITS_PER_STATION_IDX;
  region_t visitedRegions     : packed_data_structure::BITS_PER_REGION_SET;
  uint8_t visitedStationCount : packed_data_structure::BITS_FOR_VISITED_STATION_COUNT;
  float currentTime;             // could be an unsigned int, with less than 32 bits even
  float currentFuel;             // could be an unsigned int, with less than 32 bits even
  float score;                   // used to explore best labels first
};

static_assert(breitling_constraints::MANDATORY_REGION_COUNT == 4); // region_t assumes that 4 bits are enough to represent all stations

/*
 * Heuristic: when propagating labels, to avoid propagating to every other station,
 * only the K<<N nearest ones are searched. The adjency matrix is precomputed with
 * KxN entries instead of NxN.
 * 
 * If there is no refuelling station among the K nearest, the nearest refuelling station
 * is added to the list of neighbours. This might not be enough, as generated paths do
 * not go through the same station twice, it may happen that the nearest station with
 * fuel has already been crossed.
 */
struct LimitedAdjency {
  distance_t distance;
  stationidx_t station;
};

struct LabelRef {
  labelidx_t labelIndex;
  float      labelScore;
};

template<class T, class compare = std::less<T>>
class SmallBoundedPriorityQueue {
private:
  std::vector<T> m_queue;
public:
  SmallBoundedPriorityQueue(size_t capacity)
    : m_queue(capacity)
  {
  }

  const T &top()
  {
    return m_queue[0];
  }

  void pop()
  {
    assert(m_size > 0);
    m_size--;
    for (size_t i = 0; i < m_size; i++)
      m_queue[i] = m_queue[i + 1];
  }

  template<class K>
  void transferTo(K &k)
  {
    size_t capacity = m_queue.capacity();
    k = std::move(m_queue);
    m_queue.resize(capacity);
  }

  void insert(T x)
  {
    for (size_t i = 0; i < m_size; i++) {
      if (compare(m_queue[i], x)) {
        for (size_t j = std::min(m_size, S - 1); j > i; j--)
          m_queue[j] = std::move(m_queue[j - 1]);
        m_queue[i] = x;
        if (m_size != S)
          m_size++;
        return;
      }
    }
    if (m_size != S)
      m_queue[m_size++] = t;
  }
};

template<class S, typename index_t=size_t>
class ClockArenaAllocator {
protected:
  static constexpr float FREE_SLOTS_THRESHOLD = .05f; // if there are less than 5% free slots left realloc into a bigger array

  S     *m_array;
  size_t m_size;
  size_t m_next;
  size_t m_freeSlotCount;

public:
  explicit ClockArenaAllocator(size_t size)
    : m_size(size), m_next(0), m_freeSlotCount(size)
  {
    size_t capacity = sizeof(FragmentsArena) * size;
    m_array = (PathFragment *)malloc(capacity);
    assert(m_array);
    setSlotsFree(m_array, m_array + size);
  }

  ~ClockArenaAllocator()
  {
    free(m_array);
  }

  ClockArenaAllocator(const ClockArenaAllocator &) = delete;
  ClockArenaAllocator &operator=(const ClockArenaAllocator &) = delete;

  S &operator[](index_t idx) { assert(idx < m_size); return m_array[idx]; }

  index_t alloc()
  {
    m_freeSlotCount--;
    size_t previous = m_next;
    for (; m_next < m_size; m_next++) {
      if (isSlotFree(m_next))
        return m_next;
    }
    // end of the buffer reached, if there are too few free slots left realloc before cycling
    if (m_freeSlotCount < m_size * FREE_SLOTS_THRESHOLD) {
      // no more space, realloc the array
      size_t newSize = (size_t)(m_size *= 1.5f);
      PathFragment *newArray = (PathFragment *)realloc(m_array, newSize);
      if (!newArray) throw std::runtime_error("Out of memory");
      m_freeSlotCount += newSize - m_size;
      m_array = newArray;
      m_next = m_size;
      m_size = newSize;
      return m_next++;
    }

    for (m_next = 0; m_next < previous; m_next++) {
      if (isSlotFree(m_next))
        return m_next;
    }

    assert(false); // unreachable
  }

  void free(index_t slotIdx)
  {
    m_freeSlotCount++;
    setSlotsFree(&m_array[slotIdx], &m_array[slotIdx + 1]);
  }

protected:
  // overrideable
  virtual inline bool isSlotFree(const S &slot)
  {
    return *(unsigned char *)&slot == 0xff;
  }

  // overrideable
  virtual inline void setSlotsFree(const S *fromSlot, const S *toSlot)
  {
    memset(fromSlot, 0xff, toSlot - fromSlot);
  }
};

class LabelsArena : private ClockArenaAllocator<Label, labelidx_t> {
private:
  std::vector<LabelRef> m_bestLabels{20};
  float                 m_minBestScore;
public:
  explicit LabelsArena(size_t size)
    : ClockArenaAllocator(size)
  {
  }

  inline labelidx_t push(Label label)
  {
    size_t size = m_size;
    labelidx_t slot = ClockArenaAllocator::alloc();
    if (size != m_size) profiling_stats::onLabelsRealloc();
    m_array[slot] = label;
    tryInsertInBestQueue(slot);
    return slot;
  }

  const Label *queue_top()
  {
    if (m_bestLabels.empty()) {
      m_minBestScore = 0;
      for (size_t i = 0; i < m_size; i++) {
        if (!isSlotFree(m_array[i]))
          tryInsertInBestQueue(i);
      }
    }
    return m_bestLabels.empty() ? nullptr : &m_array[m_bestLabels[0].labelIndex];
  }

  void queue_pop()
  {
    assert(!m_bestLabels.empty());
    m_bestLabels.erase(m_bestLabels.begin());
  }

private:
  void tryInsertInBestQueue(labelidx_t labelIndex)
  {
    float score = m_array[labelIndex].score;
    if (score > m_minBestScore) {
      if (m_bestLabels.size() == m_bestLabels.capacity()) {
        m_minBestScore = m_bestLabels[m_bestLabels.size() - 1].labelScore;
        m_bestLabels.pop_back();
      }
      auto insertionPoint = std::upper_bound(m_bestLabels.begin(), m_bestLabels.end(), score, [](float score, const LabelRef &lr) { return score < lr.labelScore; });
      m_bestLabels.insert(insertionPoint, { labelIndex, score });
    }
  }
};

class PartialAdjencyMatrix {
private:
  std::vector<std::vector<LimitedAdjency>> m_adjencyMatrix; // NxM matrix M<<N
  std::vector<distance_t> m_distanceToTarget; // 1xN matrix, the distances to the target station must be kept somehow
  std::vector<distance_t> m_distanceToNearestRefuel;
  const ProblemMap       *m_geomap;
  const BreitlingData    *m_dataset;

public:
  PartialAdjencyMatrix(const ProblemMap *geomap, const BreitlingData *dataset)
    : m_adjencyMatrix(geomap->size()),
    m_distanceToTarget(geomap->size()),
    m_distanceToNearestRefuel(geomap->size()),
    m_geomap(geomap)
  {
    SmallBoundedPriorityQueue<LimitedAdjency> nearestStationsQueue(geomap->size()/4); // keep 1/4th of the available links

    for (stationidx_t i = 0; i < geomap->size(); i++) {
      distance_t minDistanceToFuel = std::numeric_limits<distance_t>::max();
      stationidx_t nearestStationWithFuel = -1;
      for (stationidx_t j = 0; j < geomap->size(); j++) {
        distance_t distance = utils::timeDistanceBetweenStations((*geomap)[i], (*geomap)[j], *dataset);
        nearestStationsQueue.insert({ distance, j });
        if (i == geomap->size() - 1)
          m_distanceToTarget[j] = distance;
        if ((*geomap)[j].canBeUsedToFuel() && distance < minDistanceToFuel) {
          minDistanceToFuel = distance;
          nearestStationWithFuel = j;
        }
      }
      m_distanceToNearestRefuel[i] = minDistanceToFuel;

      nearestStationsQueue.transferTo(m_adjencyMatrix[i]);
      // keep at leat one station with fuel
      if (std::find_if(m_adjencyMatrix[i].begin(), m_adjencyMatrix[i].end(), [&geomap](LimitedAdjency t) { return (*geomap)[t.station].canBeUsedToFuel(); }) == m_adjencyMatrix[i].end()) {
        m_adjencyMatrix[i].push_back({ minDistanceToFuel, nearestStationWithFuel });
      }
    }
  }

  inline distance_t distanceUncached(stationidx_t s1, stationidx_t s2)
  {
    return utils::timeDistanceBetweenStations((*m_geomap)[s1], (*m_geomap)[s2], *m_dataset);
  }

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

  inline distance_t distanceToNearestStationWithFuel(stationidx_t fromStation)
  {
    return m_distanceToNearestRefuel[fromStation];
  }

};

template<class T>
class TriangularMatrix {
private:
  T *m_array;
  size_t m_size;
public:
  TriangularMatrix(size_t size)
    : m_size(size), m_array(new T[size*(size-1)/2])
  {
  }

  ~TriangularMatrix()
  {
    delete[] m_array;
  }

  void fill(const T &value)
  {
    std::fill(m_array, m_size * (m_size - 1) / 2, value);
  }

  T &at(size_t i, size_t j)
  {
    assert(i != j);
    if (i > j) std::swap(i, j);
    size_t idx = m_size*(m_size-1)/2 - (m_size-1)*(m_size-i-1)/2 + j - i - 1;
    return m_array[idx];
  }
};

#if USE_FULL_ADJENCY_MATRIX
typedef FullAdjencyMatrix AdjencyMatrix; // maybe not completely up to date
#else
typedef PartialAdjencyMatrix AdjencyMatrix;
#endif

#if USE_SMALL_UNROLLED_PATH
typedef std::vector<bool> UnrolledPath;
#else
typedef bool *UnrolledPath;
#endif

class LabelSetting {
private:
  static constexpr region_t NO_REGION = -1;

  LabelsArena           m_labels = LabelsArena(20'000); // start with min. 20k labels, it will surely grow during execution
  AdjencyMatrix         m_adjencyMatrix;
  std::vector<region_t> m_stationRegions;
  const ProblemMap     *m_geomap;
  const BreitlingData  *m_dataset;
  distance_t            m_minDistancePerRemainingRegionCount[breitling_constraints::MANDATORY_REGION_COUNT+1];
  distance_t            m_minDistancePerRemainingStationCount[breitling_constraints::MINIMUM_STATION_COUNT+1];
  UnrolledPath          m_unrolledPathVisitedStations;

public:
  LabelSetting(const ProblemMap *geomap, const BreitlingData *dataset)
    : m_geomap(geomap), m_adjencyMatrix(geomap, dataset), m_stationRegions(geomap->size(), NO_REGION)
  {
    size_t stationCount = geomap->size();
    constexpr size_t regionCount = breitling_constraints::MANDATORY_REGION_COUNT;

    // check that the dataset is prepared
    assert(dataset->departureTime == 0);
    assert(stationCount > 0);
    assert(dataset->departureStation == 0);
    assert(dataset->targetStation == geomap->size()-1);
    if (stationCount > std::numeric_limits<stationidx_t>::max())
      throw std::runtime_error("Cannot handle that many stations");

    { // initialize station regions
      for (stationidx_t i = 0; i < stationCount; i++) {
        for (region_t r = 0; r < regionCount; r++) {
          if (breitling_constraints::isStationInMandatoryRegion(*(*geomap)[i].getOriginalStation(), r)) {
            m_stationRegions[i] = r;
            break;
          }
        }
      }
    }

    { // find good approximations for the minimal distance to cover while having explored only r<R regions

      // find the minimum distances between each regions
      TriangularMatrix<distance_t> regionAdjencyMatrix{ regionCount };
      regionAdjencyMatrix.fill(std::numeric_limits<distance_t>::max());
      for (stationidx_t i = 0; i < stationCount; i++) {
        region_t ri = m_stationRegions[i];
        if (ri == NO_REGION)
          continue;
        for (stationidx_t j = i + 1; j < stationCount; j++) {
          region_t rj = m_stationRegions[j];
          if (rj == NO_REGION)
            continue;
          distance_t &distance = regionAdjencyMatrix.at(ri, rj);
          // the adjency matrix cannot be used because it may be partial
          // and not contain the distance from i to j
          distance = std::min(distance, utils::timeDistanceBetweenStations((*geomap)[i], (*geomap)[j], *dataset));
        }
      }
      // find the R smallest distances
      SmallBoundedPriorityQueue<distance_t> sortedCentersDistances(breitling_constraints::MANDATORY_REGION_COUNT);
      for (region_t r1 = 1; r1 < regionCount; r1++) {
        for (region_t r2 = 0; r2 < r1; r2++) {
          sortedCentersDistances.insert(regionAdjencyMatrix.at(r1, r2));
        }
      }
      // accumulate the minimal distances
      // with 0 regions left to explore the minimal distance left to cover is 0
      // with 1, it is 0 + min(inter region distance)
      // with 2, it is 0 + min(inter region distance) + second min(inter region distance)
      // etc
      m_minDistancePerRemainingRegionCount[0] = 0;
      for (region_t r = 0; r < breitling_constraints::MANDATORY_REGION_COUNT; r++) {
        m_minDistancePerRemainingRegionCount[1 + r] = m_minDistancePerRemainingRegionCount[r] + sortedCentersDistances.top();
        sortedCentersDistances.pop();
      }
    }

    { // find a good approximation for the minimal distances to cover while having visited only n<N stations
      // similar method to the previous code block
      SmallBoundedPriorityQueue<distance_t> sortedDistances(breitling_constraints::MANDATORY_REGION_COUNT);
      for (stationidx_t s1 = 1; s1 < stationCount; s1++)
        for (stationidx_t s2 = 0; s2 < s1; s2++)
          sortedDistances.insert(m_adjencyMatrix.distanceUncached(s1, s2));
      m_minDistancePerRemainingStationCount[0] = 0;
      for (region_t r = 0; r < breitling_constraints::MINIMUM_STATION_COUNT; r++) {
        m_minDistancePerRemainingStationCount[1 + r] = m_minDistancePerRemainingStationCount[r] + sortedDistances.top();
        sortedDistances.pop();
      }
    }

#if USE_SMALL_UNROLLED_PATH
    m_unrolledPathVisitedStations.resize(stationCount);
#else
    m_unrolledPathVisitedStations = new bool[stationCount];
#endif
  }

  ~LabelSetting()
  {
#if !USE_SMALL_UNROLLED_PATH
    delete[] m_unrolledPathVisitedStations;
#endif
  }

private:
  time_t lowerBound(const Label &label)
  {
    unsigned char regionCountLeftToExplore = breitling_constraints::MANDATORY_REGION_COUNT - utils::countRegions(label.visitedRegions);
    distance_t minDistanceForRegions = m_minDistancePerRemainingRegionCount[regionCountLeftToExplore];
    unsigned char stationsLeftToExplore = breitling_constraints::MINIMUM_STATION_COUNT - label.visitedStationCount;
    distance_t minDistanceForStations = m_minDistancePerRemainingStationCount[stationsLeftToExplore];
    distance_t distanceToTarget = m_adjencyMatrix.distanceToTargetStation(label.currentStation);
    return std::max({ minDistanceForRegions, distanceToTarget, minDistanceForStations });
  }

  // Labels with high scores will be explored first
  float scoreLabel(const Label &label)
  {
#if 0
    // a label is good if...
    float score = 0;
    // it visited a lot of stations
    score += label.visitedStationCount * .1f;
    // it visited an appropriate number of regions
    // where "appropriate" means having explored regions at the same rate as explored stations
    score -= std::max(std::abs((float)utils::countRegions(label.visitedRegions) / breitling_constraints::MANDATORY_REGION_COUNT - (float)label.visitedStationCount / breitling_constraints::MINIMUM_STATION_COUNT)-.25f, 0.f);
    // it has fuel, but not too much
    score += label.currentFuel; // TODO score higher whenever (fuel is high)==(night is soon)
    // it is quick
    score -= label.currentTime;
    // TODO there should be factors here, the distance between stations and the plane speed/fuel capacity is ignored
    return score;
#else
    // currently there is no label domination implemented, the only reason
    // to explore one label before another is to lower the upper bound on
    // total time spent
    // to do that we score labels only depending on the number of stations
    // and regions they visited

    float score = 0;
    score += label.visitedStationCount;
    score += utils::countRegions(label.visitedRegions) * 20;
    score -= label.currentTime; // TODO find the right factor here
    return score;
#endif
  }

  void explore(const Label &source, time_t currentBestTime, std::vector<Label> &explorationLabels)
  {
    for (size_t i = 0; i < m_adjencyMatrix.adjencyCount(source.currentStation); i++) {
      stationidx_t nextStationIdx = m_adjencyMatrix.getAdjencyNextStation(source.currentStation, i);
      const ProblemStation &nextStation = (*m_geomap)[nextStationIdx];
      distance_t distanceToNext = m_adjencyMatrix.getAdjencyNextDistance(source.currentStation, i);
      region_t newLabelVisitedRegions = source.visitedRegions | m_stationRegions[nextStationIdx];

      if (source.visitedStations[nextStationIdx])
        continue; // station already visited
      if (distanceToNext > source.currentFuel)
        continue; // not enough fuel
      if (breitling_constraints::MINIMUM_STATION_COUNT - source.visitedStationCount > utils::countRegions(newLabelVisitedRegions))
        continue; // 3 regions left to visit but only 2 more stations to go through
      if (!nextStation.canBeUsedToFuel() && source.currentFuel - distanceToNext < m_adjencyMatrix.distanceToNearestStationWithFuel(nextStationIdx))
        continue; // 1 hop is possible, 2 are not because of low fuel
      if (!nextStation.isAccessibleAtNight() && utils::isTimeInNightPeriod(source.currentTime + distanceToNext, *m_dataset))
        continue; // the station is not accessible during the night

      bool shouldExploreNoRefuel = true;
      bool shouldExploreWithRefuel = nextStation.canBeUsedToFuel();

      if (m_dataset->timeToRefuel == 0 && nextStation.canBeUsedToFuel()) // if the time to refuel is 0 refuel every time it is possible
        shouldExploreNoRefuel = false;

      if (!shouldExploreNoRefuel && !shouldExploreWithRefuel)
        continue;

      if(shouldExploreNoRefuel) { // without refuel
        Label &explorationLabel = explorationLabels.emplace_back(source); // copy the source
        explorationLabel.currentFuel -= distanceToNext;
        explorationLabel.currentTime += distanceToNext;
        explorationLabel.visitedRegions = newLabelVisitedRegions;
        explorationLabel.visitedStationCount++;
        explorationLabel.visitedStations[nextStationIdx] = true;
        explorationLabel.currentStation = nextStationIdx;
        explorationLabel.score = scoreLabel(explorationLabel);
      }

      if(shouldExploreWithRefuel) { // with refuel
        Label &explorationLabel = explorationLabels.emplace_back(source);
        explorationLabel.currentFuel = utils::planeTimeFuelCapacity(*m_dataset);
        explorationLabel.currentTime += distanceToNext + m_dataset->timeToRefuel;
        explorationLabel.visitedRegions = newLabelVisitedRegions;
        explorationLabel.visitedStationCount++;
        explorationLabel.visitedStations[nextStationIdx] = true;
        explorationLabel.currentStation = nextStationIdx;
        explorationLabel.score = scoreLabel(explorationLabel);
      }
    }
  }

  std::vector<ProblemStation> reconstitutePath(const stationset_t visitedStations)
  {
    std::vector<ProblemStation> path(breitling_constraints::MINIMUM_STATION_COUNT);

    // FIX apply a path finding searching algorithm on the N used stations
    // like the TSP?
    int o = 0;
    for (int i = 0; i < visitedStations.size(); i++) {
      if (visitedStations[i])
        path[o++] = (*m_geomap)[i];
    }

    return path;
  }

  bool dominates(const Label &dominating, const Label &dominated)
  {
    return 
      (dominated.visitedStations & ~dominated.visitedStations) == 0 // the dominating label visited at least the stations visited by the dominated label
      && dominating.currentFuel >= dominated.currentFuel // the dominating has at lest as much fuel
      && dominating.currentTime <= dominated.currentTime // the dominating is at most as late
      && dominating.currentStation == dominated.currentStation // the two are at the same station // TODO use a map? to only check dominations on the same station
      // no need to check visited station/region counts as all stations visited by the
      // dominated label are also visited by the dominating label
      ;
  }

public:
  std::vector<ProblemStation> labelSetting()
  {
    time_t bestTime = std::numeric_limits<time_t>::max();
    stationset_t bestPath;

    std::vector<Label> explorationLabels(100);


    { // create the initial label
      Label initialLabel{};
      initialLabel.currentFuel = utils::planeTimeFuelCapacity(*m_dataset);
      initialLabel.currentTime = 0;
      initialLabel.currentStation = 0; // originate from station 0
      initialLabel.visitedStations[0] = true;
      initialLabel.visitedRegions = 0;
      initialLabel.visitedStationCount = 1;
      initialLabel.score = 0;
      m_labels.push(initialLabel);
    }

    while (true) {
      // take the best label currently yet-to-be-explored
      const Label *exploredPtr = m_labels.queue_top();
      if (!exploredPtr) break; // no more labels available, exit the loop and end the algorithm
      const Label &explored = *exploredPtr;
    
      // the lower bound may have been lowered since the label was added, if
      // it is no longer of intereset discard it before doing any exploration
      if (lowerBound(explored) > bestTime)
        continue;

      // explore the current label to discover new possible *and interesting* paths
      explore(explored, bestTime, explorationLabels);
      assert(explorationLabels.size() <= 127); // this is assumed by the PathFragment structure

      for (Label &nextLabel : explorationLabels) {
        if (nextLabel.visitedStationCount == breitling_constraints::MINIMUM_STATION_COUNT - 1) {
          // only 1 station remaining, try to complete the path
          distance_t distanceToComplete = m_adjencyMatrix.distanceToTargetStation(nextLabel.currentStation);
          if (nextLabel.currentFuel > distanceToComplete &&
              nextLabel.currentTime + distanceToComplete < bestTime) {
            // path is completeable and better than the best one found so far
            stationidx_t lastStation = m_geomap->size() - 1;
            bestPath = nextLabel.visitedStations;
            bestPath[lastStation] = true;
          }
        } else {
          // remove dominated labels
          auto it = explorationLabels.begin();
          while (it != explorationLabels.end()) {
            if (dominates(nextLabel, *it)) {
              it = explorationLabels.erase(it);
            } else {
              ++it;
            }
          }
          // append the new label to the open list
          m_labels.push(nextLabel);
        }
      }
      explorationLabels.clear();

      m_labels.queue_pop();
      profiling_stats::onLabelExplored();

      return reconstitutePath(bestPath);
    }
  }
};

class LabelSettingBreitlingSolver : PathSolver {
private:
  BreitlingData m_dataset;
public:
  LabelSettingBreitlingSolver(const BreitlingData &dataset)
    : m_dataset(dataset)
  {
  }

  virtual Path solveForPath(const ProblemMap &map) override
  {
    // prepare the dataset & geomap, that means having the departure 
    // station at index 0 in the geomap, the target station at index N-1
    ProblemMap preparedMap = map;
    std::swap(preparedMap[0], preparedMap[m_dataset.departureStation]);
    std::swap(preparedMap[preparedMap.size()-1], preparedMap[m_dataset.targetStation]);
    BreitlingData preparedDataset = m_dataset;
    preparedDataset.departureStation = 0;
    preparedDataset.targetStation = preparedMap.size() - 1;

    LabelSetting labelSetting{ &preparedMap, &preparedDataset };
    std::vector<ProblemStation> solvedPath = labelSetting.labelSetting();
    Path actualPath;
    actualPath.getStations().reserve(solvedPath.size());
    for (size_t i = 0; i < solvedPath.size(); i++)
      actualPath.getStations().push_back(solvedPath[i].getOriginalStation());
    return actualPath;
  }
};