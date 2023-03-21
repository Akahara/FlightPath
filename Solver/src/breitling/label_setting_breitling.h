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
 */

// see the comment on adjency matrices
#define USE_FULL_ADJENCY_MATRIX 0
// see the comment on LabelSetting::collectVisitedStations
#define USE_SMALL_UNROLLED_PATH 1

 // TODO organize classes better

typedef uint8_t stationidx_t;   // index in the Geomap
typedef uint32_t fragmentidx_t; // index in the FragmentsArena
typedef uint32_t labelidx_t;    // index in the LabelsArena
typedef float time_t;           // a duration (unit is defined by the user)
typedef time_t distance_t;      // a distance, see the comment on time/distance relation
typedef uint8_t region_t;       // bit field, 0b1010 means that the 2nd and 4th regions have been visited

// make sure that all regions combinations can be represented with region_t
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

static inline time_t timeDistanceBetweenStations(const Station &s1, const Station &s2, const BreitlingData &dataset)
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
 * Represents a node in a path.
 * 
 * The Label setting algorithm is very memory-hungry, to avoid storing label paths as arrays of stations
 * which are *way* to big, linked lists are used. A PathFragment is a node in the list, it has a station
 * id (index in the geomap's stations list), a previous fragment id (index in the FragmentsArena) and a use
 * count, the use count is used by the fragment arena to free unused fragments.
 * 
 * When a new fragment A is created and associated with a label in the open-list its use count is initialized
 * to 1. When a new fragment B is created with A as its parent, the use count of A is incremented. When the
 * label of A is explored (- the label, not its children -) the use count of A is decremented. When the
 * use count of A reaches 0 it is marked for recycling and the use count of the parent of A is decremented.
 * 
 * For example:
 * 
 * L = {C,E,F}
 * A(2) -> B(1) -> C(1)
 *      \
 *       > D(2) -> E(1)
 *              \
 *               > F(1)
 * Notation: <Fragment>(<use count>), the station id is hidden
 * 
 * If C is explored but does not produces children, its use count is decremented to 0, it is marked for
 * recycling and B's use count is decremented, also reaching 0 so it is marked to ; A's use count is
 * decremented to 1 but not marked.
 */
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

/*
 * Label of the Label Setting algorithm
 * 
 * TODO optimize the struct in size
 * TODO implement the Ir strategy
 */
struct Label {
  fragmentidx_t lastFragmentIdx;
  region_t visitedRegions;       // 4 bits
  uint8_t visitedStationCount;   // <100 -> 7 bits
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

/*
 * An arena allocator for path fragments.
 * 
 * See the documentation for PathFragment before reading this.
 * 
 * The way path fragments are stored in memory is that a *huge* buffer is allocated
 * up-front, fragments are stored in the first available slot (a slot is available
 * when it has not been allocated yet or has been marked for recycling) starting from
 * the last allocated slot. When the end of the buffer is reached is it cycled through
 * one more time, if it is completely full it is reallocated and its capacity is
 * multiplied by 1.5
 * 
 * Note that because memory is completely reallocated, pointers cannot be used to
 * reference fragments, instead fragment indices must be used. This is not too bad though
 * as pointers are way bigger than indices (on 64bit systems anyway).
 * 
 * A "marked" slot is simply set to 0xff, it is assumed that use counts cannot reach
 * that high. ie: a label cannot have more than 254 children.
 */
class FragmentsArena : private ClockArenaAllocator<PathFragment, fragmentidx_t> {
public:
  explicit FragmentsArena(size_t size)
    : ClockArenaAllocator(size)
  {
  }

  inline PathFragment &operator[](fragmentidx_t fragmentIndex)
  {
    return ClockArenaAllocator::operator[](fragmentIndex);
  }

  fragmentidx_t emplace(fragmentidx_t parent, stationidx_t station)
  {
    size_t size = m_size;
    fragmentidx_t slot = ClockArenaAllocator::alloc();
    if(size != m_size) profiling_stats::onFragmentsRealloc();
    m_array[parent].setUseCount(m_array[parent].getUseCount() + 1); // increment parent use count
    new (&m_array[slot]) PathFragment(station, parent);             // construct in place
    return slot;
  }

  void release(fragmentidx_t fragmentIdx)
  {
    PathFragment &fragment = m_array[fragmentIdx];
    assert(!isSlotFree(fragment));
    size_t newUseCount = fragment.getUseCount() - 1ull;
    if (newUseCount != 0) {
      fragment.setUseCount(newUseCount);
    } else {
      if (fragment.getStationIdx() != 0)
        release(fragment.getPreviousFragment());
      memset(&fragment, 0xff, sizeof(PathFragment)); // equivalent to free(fragment)
    }
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

public:
  PartialAdjencyMatrix(const GeoMap &geomap, const BreitlingData &dataset)
    : m_adjencyMatrix(geomap.getStations().size()),
    m_distanceToTarget(geomap.getStations().size()),
    m_distanceToNearestRefuel(geomap.getStations().size())
  {
    SmallBoundedPriorityQueue<LimitedAdjency> nearestStationsQueue(geomap.getStations().size()/4); // keep 1/4th of the available links

    for (stationidx_t i = 0; i < geomap.getStations().size(); i++) {
      distance_t minDistanceToFuel = std::numeric_limits<distance_t>::max();
      stationidx_t nearestStationWithFuel = -1;
      for (stationidx_t j = 0; j < geomap.getStations().size(); j++) {
        distance_t distance = utils::timeDistanceBetweenStations(geomap.getStations()[i], geomap.getStations()[j], dataset);
        nearestStationsQueue.insert({ distance, j });
        if (i == geomap.getStations().size() - 1)
          m_distanceToTarget[j] = distance;
        if (geomap.getStations()[j].hasFuel() && distance < minDistanceToFuel) {
          minDistanceToFuel = distance;
          nearestStationWithFuel = j;
        }
      }
      m_distanceToNearestRefuel[i] = minDistanceToFuel;

      nearestStationsQueue.transferTo(m_adjencyMatrix[i]);
      if (std::find_if(m_adjencyMatrix[i].begin(), m_adjencyMatrix[i].end(), [&geomap](LimitedAdjency t) { return geomap.getStations()[t.station].hasFuel(); }) == m_adjencyMatrix[i].end()) {
        m_adjencyMatrix[i].push_back({ minDistanceToFuel, nearestStationWithFuel });
      }
    }
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

class FullAdjencyMatrix {
private:
  std::vector<distance_t> m_adjencyMatrix; // unrolled NxN matrix
  size_t                  m_nodeCount;
  // TODO could be stored as a triangle "matrix" (only half of it) because the NxN matrix is symetric, todo is to check performances

public:
  FullAdjencyMatrix(const GeoMap &geomap, const BreitlingData &dataset)
    : m_nodeCount(geomap.getStations().size())
  {
    m_adjencyMatrix.resize(m_nodeCount * m_nodeCount);
    for (size_t i = 0; i < m_nodeCount; i++) {
      for (size_t j = i; j < m_nodeCount; j++) {
        m_adjencyMatrix[i + j * m_nodeCount] =
        m_adjencyMatrix[i * m_nodeCount + j] =
          utils::timeDistanceBetweenStations(geomap.getStations()[i], geomap.getStations()[j], dataset);
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
    return m_adjencyMatrix[currentStation * m_nodeCount*idx];
  }

  inline distance_t distanceToTargetStation(stationidx_t fromStation)
  {
    return m_adjencyMatrix[m_adjencyMatrix.size()-1 + m_nodeCount*fromStation];
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

  FragmentsArena        m_fragments = FragmentsArena(100'000); // start with min. 100k fragments, it will surely grow during execution
  LabelsArena           m_labels = LabelsArena(20'000); // start with min. 20k labels, it will surely grow during execution
  AdjencyMatrix         m_adjencyMatrix;
  std::vector<region_t> m_stationRegions;
  const GeoMap         *m_geomap;
  const BreitlingData  *m_dataset;
  distance_t            m_minDistancePerRemainingRegionCount[breitling_constraints::MANDATORY_REGION_COUNT+1];
  UnrolledPath          m_unrolledPathVisitedStations;

public:
  LabelSetting(const GeoMap *geomap, const BreitlingData *dataset)
    : m_geomap(geomap), m_adjencyMatrix(*geomap, *dataset), m_stationRegions(geomap->getStations().size(), NO_REGION)
  {
    // TODO prepare the dataset & geomap
    // that means having the departure station at index 0 in the geomap,
    // the target station at index N-1, the departure time at 0 and
    // day/night time offseted to reflect the departure time change.

    size_t stationCount = geomap->getStations().size();
    constexpr size_t regionCount = breitling_constraints::MANDATORY_REGION_COUNT;

    // check that the dataset is prepared
    assert(dataset->departureTime == 0);
    assert(stationCount > 0);
    assert(&geomap->getStations()[0] == dataset->departureStation);
    assert(&geomap->getStations()[geomap->getStations().size()-1] == dataset->targetStation);
    if (stationCount > std::numeric_limits<stationidx_t>::max())
      throw std::runtime_error("Cannot handle that many stations");

    { // initialize station regions
      for (stationidx_t i = 0; i < stationCount; i++) {
        for (region_t r = 0; r < regionCount; r++) {
          if (breitling_constraints::isStationInMandatoryRegion(geomap->getStations()[i], r)) {
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
          distance = std::min(distance, utils::timeDistanceBetweenStations(geomap->getStations()[i], geomap->getStations()[j], *dataset));
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
    distance_t distanceToTarget = m_adjencyMatrix.distanceToTargetStation(m_fragments[label.lastFragmentIdx].getStationIdx());
    // TODO create a distance lower bound based on the number of stations left to visit
    return std::max({ minDistanceForRegions, distanceToTarget });
  }

  /*
   * Returns a vector with stationCount elements.
   * Element i is set to true iff the given path crossed station i.
   * 
   * Depending on USE_SMALL_UNROLLED_PATH the vector can be a bool* with
   * higher memory overhead but lower access time or a std::vector<bool>
   * using CHAR_BIT=8 times less memory per member but with higher access
   * time. The two changes are inconsequencial, there are too few stations
   * for the size to matter and the access time of std::vector<bool> is
   * tiny enough.
   */
  const UnrolledPath &collectVisitedStations(fragmentidx_t pathEnd)
  {
#if USE_SMALL_UNROLLED_PATH
    std::fill(m_unrolledPathVisitedStations.begin(), m_unrolledPathVisitedStations.end(), 0);
#else
    memset(m_unrolledPathVisitedStations, 0, sizeof(bool) * m_geomap->getStations().size());
#endif
    while (pathEnd != 0) {
      m_unrolledPathVisitedStations[m_fragments[pathEnd].getStationIdx()] = true;
      pathEnd = m_fragments[pathEnd].getPreviousFragment();
    }
    m_unrolledPathVisitedStations[0] = true;
  }

  float scoreLabel(const Label &label)
  {
    // Labels with high scores will be explored first
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
    PathFragment &currentFragment = m_fragments[source.lastFragmentIdx];
    stationidx_t currentStation = currentFragment.getStationIdx();
    const UnrolledPath &visitedStations = collectVisitedStations(source.lastFragmentIdx);
    for (size_t i = 0; i < m_adjencyMatrix.adjencyCount(currentStation); i++) {
      stationidx_t nextStationIdx = m_adjencyMatrix.getAdjencyNextStation(currentStation, i);
      const Station &nextStation = m_geomap->getStations()[nextStationIdx];
      distance_t distanceToNext = m_adjencyMatrix.getAdjencyNextDistance(currentStation, i);
      region_t newLabelVisitedRegions = source.visitedRegions | m_stationRegions[nextStationIdx];

      if (visitedStations[nextStationIdx])
        continue; // station already visited
      if (distanceToNext > source.currentFuel)
        continue; // not enough fuel
      if (breitling_constraints::MINIMUM_STATION_COUNT - source.visitedStationCount > utils::countRegions(newLabelVisitedRegions))
        continue; // 3 regions left to visit but only 2 more stations to go through
      if (!nextStation.hasFuel() && source.currentFuel - distanceToNext < m_adjencyMatrix.distanceToNearestStationWithFuel(nextStationIdx))
        continue; // 1 hop is possible, 2 are not because of low fuel
      if (!nextStation.isAccessibleAtNight() && utils::isTimeInNightPeriod(source.currentTime + distanceToNext, *m_dataset))
        continue; // the station is not accessible during the night

      bool shouldExploreNoRefuel = true;
      bool shouldExploreWithRefuel = nextStation.hasFuel();

      if (m_dataset->timeToRefuel == 0 && nextStation.hasFuel()) // if the time to refuel is 0 refuel every time it is possible
        shouldExploreNoRefuel = false;

      if (!shouldExploreNoRefuel && !shouldExploreWithRefuel)
        continue;

      fragmentidx_t newPathFragment = m_fragments.emplace(source.lastFragmentIdx, nextStationIdx);

      if(shouldExploreNoRefuel) { // without refuel
        Label &explorationLabel = explorationLabels.emplace_back(source); // copy the source
        explorationLabel.currentFuel -= distanceToNext;
        explorationLabel.currentTime += distanceToNext;
        explorationLabel.visitedRegions = newLabelVisitedRegions;
        explorationLabel.visitedStationCount++;
        explorationLabel.lastFragmentIdx = newPathFragment;
        explorationLabel.score = scoreLabel(explorationLabel);
      }

      if(shouldExploreWithRefuel) { // with refuel
        Label &explorationLabel = explorationLabels.emplace_back(source);
        explorationLabel.currentFuel = utils::planeTimeFuelCapacity(*m_dataset);
        explorationLabel.currentTime += distanceToNext + m_dataset->timeToRefuel;
        explorationLabel.visitedRegions = newLabelVisitedRegions;
        explorationLabel.visitedStationCount++;
        explorationLabel.lastFragmentIdx = newPathFragment;
        explorationLabel.score = scoreLabel(explorationLabel);
      }
    }
  }

  //bool dominates(Label &dominating, Label &dominated)
  //{
  //  // TODO implement
  //  return false;
  //}

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

    std::vector<Label> explorationLabels(100);

    { // create the initial label
      Label initialLabel{};
      initialLabel.currentFuel = utils::planeTimeFuelCapacity(*m_dataset);
      initialLabel.currentTime = 0;
      initialLabel.lastFragmentIdx = m_fragments.emplace(0, 0); // originate from station 0
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
          //// remove dominated labels
          //auto it = openLabels.begin();
          //while (it != openLabels.end()) {
          //  if (dominates(nextLabel, *it)) {
          //    m_fragments.release(it->lastFragmentIdx);
          //    it = openLabels.erase(it);
          //  } else {
          //    ++it;
          //  }
          //}
          // append the new label to the open list
          m_labels.push(nextLabel);
        }
      }
      explorationLabels.clear();

      m_fragments.release(explored.lastFragmentIdx);
      m_labels.queue_pop();
      profiling_stats::onLabelExplored();

      return reconstitutePath(bestPath);
    }
  }
};