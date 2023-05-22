#include "label_setting_breitling.h"

#ifndef _DEBUG
#define NDEBUG
#endif

#include <cstdlib>
#include <cstring>
#include <stdint.h>
#include <assert.h>
#include <new>
#include <stdexcept>
#include <vector>
#include <list>
#include <array>
#include <bitset>
#include <algorithm>
#include <iostream>
#include <map>
#include <unordered_map>
#include <fstream>

#include "../geometry.h"
#include "breitlingnatural.h"
#include "structures.h"

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

/*
 * Ir strategy
 * 
 * Given a label L1 with explored regions e_i\in{0,1} at station s1, R_i the set of stations
 * in region i and (I_i) a partition of stations where s\in R_i\rarrow s\in I_i,
 * that is, I_i areas are covering all stations and R_i is included in I_i.
 * Then, for an explorable label L2 with explored regions e'_i at station s2
 * - if e_{region of s1} = 0 explore L2 iff s2\in I_{region of s1}
 * - if e_{region of s1} = 1 explore L2 iff \forall r,e_r=1\and r\neq region of s1 \rightarrow s2 \notin I_r
 * that is, if if L1 is in extended region I_r but did not explore R_r then
 * only explore labels in I_r. And if L1 already explored I_r then do not
 * explore already explored I_r.
 */

/*
 * Label setting
 * 
 * S = {initial label} // discovered labels, takes part is domination checks
 * R = {initial label} // explorable labels, sorted by score
 * besttime = +inf
 * bestlabel = null
 * while R is not empty
 *   pick the label L with the highest score
 *   remove L from R
 *   if lowerbound(L) > besttime
 *     continue
 *   E = explore(L)
 *    for L' in E
 *      if there is a L" in S dominating L'
 *        continue
 *      for all labels L" dominated by L'
 *        remove L" from S and from R if it is was there
 *        continue
 *      if L' is acceptable
 *        if time(L') < besttime
 *          besttime = time(L')
 *          bestlabel = L'
 *      else
 *        add L' to S and R
 * return bestlabel
 */

/* when set, if too many labels are discovered the exploration queue will
 * not grow to fit them all, they will still be stored to do domination
 * checks. When not set the queue contains a cache of the best labels, 
 * when the cache is empty it is refilled by reading *all* alive labels */
#define LIMITED_CONCURRENT_LABELS 10000 
/* when defined a quick heuristic is used to find a path(see breitlingnatural.cpp)
 * the found path length is used as a lower bound for the label setting */
//#define USE_HEURISTIC_LOWER_BOUND
/* maximum search time, in seconds */
//#define LIMITED_SEARCH_TIME 15


/**
 * constants here are define to optimize structure sizes
 * By assuming number constraints (such as 'there will be no more than N=512 stations total')
 * we can know the exact number of bits that are required to represent these numbers.
 * In C++ we can write 'int i : 3' to tell the compiler to only use 3 bits, but it is
 * considered as a hint rather than as a constraints, because of byte-packing and padding
 * we don't actually know for sure that 'int a:3,b:5' will use 8 bits, instead we can
 * pack values ourselves 'char ab' and read/write to a and b using methods with bitwise
 * operators (shifting, and bit masks).
 */
namespace packed_data_structures {

constexpr size_t MAX_SUPPORTED_STATIONS = 512;
constexpr size_t BITS_PER_STATION_IDX = std::bit_width(MAX_SUPPORTED_STATIONS - 1);
constexpr size_t MAX_REGION_COUNT = breitling_constraints::MANDATORY_REGION_COUNT;
constexpr size_t BITS_PER_REGION_SET = MAX_REGION_COUNT;
constexpr size_t BITS_FOR_VISITED_STATION_COUNT = std::bit_width(breitling_constraints::MINIMUM_STATION_COUNT - 1);

}

typedef uint16_t stationidx_t;  // index in the Geomap
typedef uint32_t labelidx_t;    // index in the LabelsArena
typedef uint32_t fragmentidx_t; // index in the FragmentsArena
typedef float disttime_t;       // a time duration or a distance, see the comment on time/dist
typedef uint8_t region_t;       // bit field, 0b1010 means that the 2nd and 4th regions have been visited
typedef uint8_t regionidx_t;    // offset in a region_t, ie. regionidx_t=2 means the third region and corresponds to region_t=0b100
typedef float score_t;          // a label score, labels with higher scores are explored first
typedef SpecificBitSet<packed_data_structures::MAX_SUPPORTED_STATIONS> StationSet;

// failsafe, a runtime check is also necessary to validate that MAX_SUPPORTED_STATIONS is high enough
static_assert(breitling_constraints::MINIMUM_STATION_COUNT < packed_data_structures::MAX_SUPPORTED_STATIONS);
// all regions combinations can be represented with region_t
static_assert(breitling_constraints::MANDATORY_REGION_COUNT < sizeof(region_t) * CHAR_BIT);
// stationidx_t is large enough
static_assert(packed_data_structures::MAX_SUPPORTED_STATIONS <= std::numeric_limits<stationidx_t>::max());



namespace utils {

static constexpr std::array BIT_COUNT_LOOKUP_TABLE = createBitCountLookupTable<1 << breitling_constraints::MANDATORY_REGION_COUNT>();
static constexpr std::array FIRST_BIT_INDEX_LOOKUP_TABLE = createFirstBitLookupTable<1 << breitling_constraints::MANDATORY_REGION_COUNT>();

// Returns the number of visited regions in a region bit field
// ie. 0b1010 represents regions 2 & 4 being visited, countRegions(0b1010) = 2
static unsigned char countRegions(region_t regionsBitField)
{
  return BIT_COUNT_LOOKUP_TABLE[regionsBitField];
}

// Returns the index of the first visited regions in a region bit field
// ie. 0b0010 represents region 2 (index 1) being visited, firstRegionSetIndex(0b0010) = 1
static uint8_t firstRegionSetIndex(region_t regionsBitField)
{
  return FIRST_BIT_INDEX_LOOKUP_TABLE[regionsBitField];
}

static inline disttime_t timeDistanceBetweenStations(const ProblemStation &s1, const ProblemStation &s2, const BreitlingData &dataset)
{
  nauticmiles_t realDistance = geometry::distance(s1.getLocation(), s2.getLocation());
  disttime_t timeDistance = realDistance / dataset.planeSpeed;
  return timeDistance;
}

static inline disttime_t realDistanceToTimeDistance(nauticmiles_t realDistance, const BreitlingData &dataset)
{
  return realDistance / dataset.planeSpeed;
}

static inline disttime_t getPlaneAutonomy(const BreitlingData &dataset)
{
  return dataset.planeFuelCapacity / dataset.planeFuelUsage;
}

/*
 * Assumes that 0..nauticalDaytime and nauticalNighttime..24 are night periods,
 * that is, 0 < nauticalDaytime < nauticalNighttime < 24
 */
static inline bool isTimeInNightPeriod(disttime_t time, const BreitlingData &dataset)
{
  time = fmod(time, 24.f);
  return time < dataset.nauticalDaytime || time > dataset.nauticalNighttime;
}

} // !namespace utils

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
public:
  static constexpr fragmentidx_t NO_PARENT_FRAGMENT = 0;
  static constexpr size_t MAX_CHILDREN_COUNT = 127;
private:
  static constexpr fragmentidx_t PREVIOUS_FRAGMENT_IDX_EMPTY_MARKER = -1;

  // 9 bits for the station (up to 512) and 7 bits for the use count (up to 127 immediate children + 1 immediate use)
  uint16_t packedStationUseCount;
  fragmentidx_t previousFragmentIdx;

public:
  PathFragment(stationidx_t station, fragmentidx_t previousFragmentIdx)
    : previousFragmentIdx(previousFragmentIdx)
  {
    assert(station <= 0b1'1111'1111);
    packedStationUseCount = station | (1 << 9); // initialized with 1 use
  }

  inline stationidx_t getStationIdx() const { return packedStationUseCount & 0b1'1111'1111; }
  inline uint8_t getUseCount() const { return packedStationUseCount >> 9; }
  inline void setUseCount(uint8_t count) { assert(count <= 0b111'111); packedStationUseCount = getStationIdx() | count << 9; }
  inline fragmentidx_t getPreviousFragment() const { return previousFragmentIdx; }

  inline bool isEmpty() const { return previousFragmentIdx == PREVIOUS_FRAGMENT_IDX_EMPTY_MARKER; }
  inline void setEmpty() { previousFragmentIdx = PREVIOUS_FRAGMENT_IDX_EMPTY_MARKER; }
};

/*
 * Label of the Label Setting algorithm
 *
 * TODO optimize the struct in size
 */
struct Label {
  static constexpr fragmentidx_t NO_FRAGMENT = PathFragment::NO_PARENT_FRAGMENT;
  
  // the label's current position (the last station it got to)
  stationidx_t  currentStation : packed_data_structures::BITS_PER_STATION_IDX;
  // bitset of the visited regions, see #region_t
  region_t      visitedRegions : packed_data_structures::BITS_PER_REGION_SET;
  // the number of visited stations, should never go higher than breitling_constraints::MANDATORY_STATION_COUNT
  uint8_t       visitedStationCount : packed_data_structures::BITS_FOR_VISITED_STATION_COUNT;
  // could be an unsigned int, with less than 32 bits even
  disttime_t    currentTime;
  // could be an unsigned int, with less than 32 bits even
  disttime_t    currentFuel;
  // used to explore best labels first, see LabelSetting::scoreLabel()
  score_t       score;
  // bitset of the visited stations, used to not visit the same station twice
  StationSet    visitedStations;
  // the label's latest path fragment, used to restore a path from a label
  fragmentidx_t pathFragment = NO_FRAGMENT;

  // used as NaN value in labels priority queues
  static constexpr score_t MIN_POSSIBLE_SCORE = std::numeric_limits<float>::lowest() * .5f;
  // used to mark label memory slots as empty in label pools
  static constexpr score_t EMPTY_SCORE_MARKER = std::numeric_limits<float>::lowest() * .95f;
  // used to mark a label as explored but not yet freed in label pools
  static constexpr score_t EXPLORED_SCORE_MARKER = std::numeric_limits<float>::lowest();
  static_assert(MIN_POSSIBLE_SCORE > EMPTY_SCORE_MARKER);
  static_assert(MIN_POSSIBLE_SCORE > EXPLORED_SCORE_MARKER);

  inline bool isExplorable() const { return score == EXPLORED_SCORE_MARKER; }
  inline void setExplored() { score = EXPLORED_SCORE_MARKER; }
  inline bool isEmpty() const { return score == EMPTY_SCORE_MARKER; }
  inline void setEmpty() { score = EMPTY_SCORE_MARKER; }
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
  disttime_t distance;
  stationidx_t station;
};

struct LabelInlineAllocator {
  inline bool isFree(const Label &slot) const
  {
    return slot.isEmpty();
  }

  inline void setFree(Label *slot)
  {
    slot->setEmpty();
  }

  inline void setFree(Label *fromSlot, Label *toSlot)
  {
    while (fromSlot < toSlot)
      setFree(--toSlot);
  }
};

class LabelsArena : public ClockArenaAllocator<Label, labelidx_t, LabelInlineAllocator> {
public:
  explicit LabelsArena(size_t size)
    : ClockArenaAllocator(size)
  {
  }

  inline labelidx_t push(Label label)
  {
    size_t size = m_size;
    labelidx_t slot = ClockArenaAllocator::alloc();
    if (size != m_size)
      PROFILING_COUNTER_INC(labels_realloc);
    m_array[slot] = label;
    return slot;
  }
};

struct PathFragmentInlineAllocator {
  inline bool isFree(const PathFragment &fragment) const
  {
    return fragment.isEmpty();
  }

  inline void setFree(PathFragment *fragment)
  {
    fragment->setEmpty();
  }

  inline void setFree(PathFragment *fromSlot, PathFragment *toSlot)
  {
    while (fromSlot < toSlot)
      setFree(--toSlot);
  }
};

class PathFragmentsArena : ClockArenaAllocator<PathFragment, fragmentidx_t, PathFragmentInlineAllocator> 
{
public:
  explicit PathFragmentsArena(size_t size)
    : ClockArenaAllocator(size)
  {
  }

  inline fragmentidx_t pushInitial(stationidx_t station)
  {
    fragmentidx_t slot = ClockArenaAllocator::alloc();
    m_array[slot] = PathFragment(station, PathFragment::NO_PARENT_FRAGMENT);
    return slot;
  }

  inline fragmentidx_t push(stationidx_t station, fragmentidx_t parent)
  {
    assert(parent != -1 && !m_allocator.isFree(m_array[parent]));
    fragmentidx_t slot = ClockArenaAllocator::alloc();
    m_array[slot] = PathFragment(station, parent);
    m_array[parent].setUseCount(m_array[parent].getUseCount() + 1);
    return slot;
  }

  inline void release(fragmentidx_t fragmentIdx)
  {
    if (fragmentIdx == Label::NO_FRAGMENT)
      return;

    PathFragment &fragment = m_array[fragmentIdx];
    assert(!m_allocator.isFree(fragment));

    uint8_t newUseCount = fragment.getUseCount()-1;
    if (newUseCount == 0) {
      //std::cout << "R(" << fragmentIdx << ")";
      assert(fragmentIdx != PathFragment::NO_PARENT_FRAGMENT);
      if(fragment.getPreviousFragment() != PathFragment::NO_PARENT_FRAGMENT)
        release(fragment.getPreviousFragment());
      ClockArenaAllocator::free(fragmentIdx);
    } else {
      fragment.setUseCount(newUseCount);
    }
  }

  const PathFragment &operator[](fragmentidx_t idx)
  {
    return ClockArenaAllocator::operator[](idx);
  }
};

/*
 * Best labels queues do not posess labels, only label references.
 * A reference contains the label score, this is to avoid random memory
 * accesses while sorting label references, it avoid fetching from the
 * LabelsArena.
 */
struct LabelRef {
  labelidx_t labelIndex;
  float      labelScore;
};

#ifdef LIMITED_CONCURRENT_LABELS
class BestLabelsQueue {
private:
  struct LabelRefComp {
    bool operator()(const LabelRef &l1, const LabelRef &l2)
    {
      return l1.labelScore > l2.labelScore;
    }
  };

private:
  SmallBoundedPriorityQueue<LabelRef, LabelRefComp> m_bestLabels;
  const LabelsArena *m_labelsArena;

public:
  BestLabelsQueue(const LabelsArena *labelsArena)
    : m_bestLabels(LIMITED_CONCURRENT_LABELS), m_labelsArena(labelsArena)
  {
  }

  void remove(labelidx_t labelIndex)
  {
    m_bestLabels.removeIf([labelIndex](const LabelRef &r) { return r.labelIndex == labelIndex; });
  }

  labelidx_t popFront()
  {
    if (m_bestLabels.empty())
      return -1;

    labelidx_t front = m_bestLabels.top().labelIndex;
    m_bestLabels.pop();
    return front;
  }

  void tryInsertInQueue(labelidx_t labelIndex)
  {
    float score = (*m_labelsArena)[labelIndex].score;
    m_bestLabels.insert({ labelIndex, score });
  }
};
#else
class BestLabelsQueue {
private:
  std::vector<LabelRef> m_bestLabels;
  score_t               m_minBestScore;
  const LabelsArena    *m_labelsArena;
public:
  BestLabelsQueue(const LabelsArena *labelsArena)
    : m_labelsArena(labelsArena), m_minBestScore(Label::MIN_POSSIBLE_SCORE)
  {
    constexpr size_t cacheSize = 1000;
    m_bestLabels.reserve(cacheSize);
  }

  void remove(labelidx_t labelIndex)
  {
    for (size_t i = 0; i < m_bestLabels.size(); i++) {
      if (m_bestLabels[i].labelIndex == labelIndex) {
        m_bestLabels.erase(m_bestLabels.begin() + i);
        return;
      }
    }
  }

  labelidx_t popFront()
  {
    if (m_bestLabels.empty()) {
      PROFILING_COUNTER_INC(best_labels_pool_cache_miss);
      // the end of the "best labels" cache has been reached,
      // go through all labels to repopulate the cache with
      // the N best ones
      m_minBestScore = Label::MIN_POSSIBLE_SCORE;
      for (labelidx_t i = m_labelsArena->firstValidIndex(); i < m_labelsArena->getMaxIndex(); i = m_labelsArena->nextValidIndex(i)) { // TODO FUTURE replace with iterators
        tryInsertInQueue(i);
      }
      // still no labels, the end of the queue has been reached
      if (m_bestLabels.empty())
        return -1;
    }

    labelidx_t front = m_bestLabels[0].labelIndex;
    m_bestLabels.erase(m_bestLabels.begin());
    return front;
  }

  void tryInsertInQueue(labelidx_t labelIndex)
  {
    float score = (*m_labelsArena)[labelIndex].score;
    if (score > m_minBestScore) {
      if (m_bestLabels.size() == m_bestLabels.capacity()) {
        m_minBestScore = m_bestLabels[m_bestLabels.size() - 1].labelScore;
        m_bestLabels.pop_back();
      }
      auto insertionPoint = std::upper_bound(m_bestLabels.begin(), m_bestLabels.end(), score, [](float score, const LabelRef &lr) { return score > lr.labelScore; });
      m_bestLabels.insert(insertionPoint, { labelIndex, score });
    }
  }
};
#endif

class PartialAdjencyMatrix {
private:
  struct LimitedAdjencyComparator {
    bool operator()(const LimitedAdjency &l1, const LimitedAdjency &l2) { return l1.distance < l2.distance; }
  };

private:
  std::vector<std::vector<LimitedAdjency>> m_adjencyMatrix; // NxM matrix M<<N
  std::vector<disttime_t> m_distanceToTarget; // 1xN matrix, the distances to the target station must be kept somehow
  std::vector<disttime_t> m_distanceToNearestRefuel;
  const ProblemMap *m_geomap;
  const BreitlingData *m_dataset;

public:
  PartialAdjencyMatrix(const ProblemMap *geomap, const BreitlingData *dataset, stationidx_t targetStation)
    : m_adjencyMatrix(geomap->size()),
    m_distanceToTarget(geomap->size()),
    m_distanceToNearestRefuel(geomap->size()),
    m_geomap(geomap),
    m_dataset(dataset)
  {
    SmallBoundedPriorityQueue<LimitedAdjency, LimitedAdjencyComparator> nearestStationsQueue(20); // keep only the 20 shortest links per station

    for (stationidx_t i = 0; i < geomap->size(); i++) {
      disttime_t minDistanceToFuel = std::numeric_limits<disttime_t>::max();
      stationidx_t nearestStationWithFuel = -1;
      for (stationidx_t j = 0; j < geomap->size(); j++) {
        if (i == j)
          continue;
        disttime_t distance = utils::timeDistanceBetweenStations((*geomap)[i], (*geomap)[j], *dataset);
        if (j == targetStation)
          m_distanceToTarget[i] = distance;
        else // do not use the target station as a neighbour of any station
          nearestStationsQueue.insert({ distance, j });
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

  inline disttime_t distanceUncached(stationidx_t s1, stationidx_t s2)
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

  inline disttime_t getAdjencyNextDistance(stationidx_t currentStation, size_t idx)
  {
    return m_adjencyMatrix[currentStation][idx].distance;
  }

  inline disttime_t distanceToTargetStation(stationidx_t fromStation)
  {
    return m_distanceToTarget[fromStation];
  }

  inline disttime_t distanceToNearestStationWithFuel(stationidx_t fromStation)
  {
    return m_distanceToNearestRefuel[fromStation];
  }

};

#if 1
static void writeStations(const StationSet &stationSet, stationidx_t lastStation, const ProblemMap &map, const std::string &outfile="debug_stations.svg")
{
  std::ofstream file{ outfile };

  double
    minLon = std::numeric_limits<double>::max(),
    minLat = std::numeric_limits<double>::max(),
    maxLon = std::numeric_limits<double>::min(),
    maxLat = std::numeric_limits<double>::min();
  for (const ProblemStation &station : map) {
    double lon = station.getLocation().lon;
    double lat = station.getLocation().lat;
    minLon = std::min(minLon, lon);
    minLat = std::min(minLat, lat);
    maxLon = std::max(maxLon, lon);
    maxLat = std::max(maxLat, lat);
  }

  constexpr double padding = 1;
  file << "<svg viewBox=\""
    << (minLon - padding) << " "
    << (minLat - padding) << " "
    << (maxLon - minLon + 2 * padding) << " "
    << (maxLat - minLat + 2 * padding)
    << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";

  for (stationidx_t i = 0; i < map.size(); i++) {
    const Location &loc = map[i].getLocation();
    file
      << "<circle cx=\"" << loc.lon << "\" cy=\"" << loc.lat << "\" r=\".15\" "
      << "fill=\"" << (i == lastStation ? "blue" : (stationSet.isSet(i) ? "green" : "black")) << "\"/>\n";
  }

  file << "</svg>" << std::endl;
}
#endif


class LabelSetting {
private:
  static constexpr region_t NO_REGION = 0;

  const ProblemMap     *m_geomap;
  const BreitlingData  *m_dataset;

  PartialAdjencyMatrix  m_adjencyMatrix;
  std::vector<region_t> m_stationRegions;
  std::vector<region_t> m_stationExtendedRegions;

  disttime_t            m_minDistancePerRemainingRegionCount[breitling_constraints::MANDATORY_REGION_COUNT + 1];
  disttime_t            m_minDistancePerRemainingStationCount[breitling_constraints::MINIMUM_STATION_COUNT + 1];

  LabelsArena           m_labels = LabelsArena(20'000); // start with min. 20k labels, it will surely grow during execution
  PathFragmentsArena    m_fragments = PathFragmentsArena(20'000);
  BestLabelsQueue       m_bestLabelsQueue;
  std::vector<std::vector<labelidx_t>> m_labelsPerStationsIndex;

  disttime_t            m_noBestTime = std::numeric_limits<disttime_t>::max();
  disttime_t            m_bestTime = m_noBestTime;

public:
  LabelSetting(const ProblemMap *geomap, const BreitlingData *dataset)
    : m_geomap(geomap),
    m_adjencyMatrix(geomap, dataset, dataset->targetStation),
    m_stationRegions(geomap->size(), NO_REGION),
    m_stationExtendedRegions(geomap->size(), NO_REGION),
    m_dataset(dataset),
    m_labelsPerStationsIndex(geomap->size()),
    m_bestLabelsQueue(&m_labels)
  {
    size_t stationCount = geomap->size();
    constexpr size_t regionCount = breitling_constraints::MANDATORY_REGION_COUNT;

    // check that the dataset is prepared
    assert(stationCount > 0);
    if (stationCount > packed_data_structures::MAX_SUPPORTED_STATIONS)
      throw std::runtime_error("Cannot handle that many stations");

    { // initialize station regions and extended regions
      struct GravityCenter { double accLon = 0; double accLat = 0; size_t stationCount = 0; Location location; };
      std::array<GravityCenter, regionCount> regionsCenters;

      for (stationidx_t i = 0; i < stationCount; i++) {
        const ProblemStation &station = (*geomap)[i];
        for (regionidx_t r = 0; r < regionCount; r++) {
          if (breitling_constraints::isStationInMandatoryRegion(*station.getOriginalStation(), r)) {
            // set the station's region
            m_stationRegions[i] = 1<<r;
            // move the region's gravity center
            regionsCenters[r].accLon += station.getLocation().lon;
            regionsCenters[r].accLat += station.getLocation().lat;
            regionsCenters[r].stationCount++;
            break;
          }
        }
      }

      // average all centers
      for (GravityCenter &center : regionsCenters) {
        if (center.stationCount == 0)
          throw std::runtime_error("A mandatory region is empty");
        center.location = Location{ center.accLon / center.stationCount, center.accLat / center.stationCount };
      }

      // set stations' extended regions by finding the nearest centers
      // works because regions are convex, do not overlap and have almost the same size
      for (stationidx_t i = 0; i < stationCount; i++) {
        disttime_t minDist = std::numeric_limits<disttime_t>::max();
        regionidx_t extendedRegion = -1;
        const ProblemStation &station = (*geomap)[i];
        for (regionidx_t r = 0; r < regionCount; r++) {
          disttime_t dist = geometry::distance(regionsCenters[r].location, station.getLocation());
          if (dist < minDist) {
            minDist = dist;
            extendedRegion = r;
          }
        }
        m_stationExtendedRegions[i] = 1<<extendedRegion;
      }
    }

    { // find good approximations for the minimal distance to cover while having explored only r<R regions

      // find the minimum distances between each regions
      TriangularMatrix<disttime_t> regionAdjencyMatrix{ regionCount };
      regionAdjencyMatrix.fill(std::numeric_limits<disttime_t>::max());
      for (stationidx_t i = 0; i < stationCount; i++) {
        region_t ri = m_stationRegions[i];
        if (ri == NO_REGION)
          continue;
        regionidx_t riidx = utils::firstRegionSetIndex(ri);
        for (stationidx_t j = i + 1; j < stationCount; j++) {
          region_t rj = m_stationRegions[j];
          if (rj == NO_REGION || ri == rj)
            continue;
          regionidx_t rjidx = utils::firstRegionSetIndex(rj);
          disttime_t &distance = regionAdjencyMatrix.at(riidx, rjidx);
          // the adjency matrix cannot be used because it may be partial
          // and not contain the distance from i to j
          distance = std::min(distance, utils::timeDistanceBetweenStations((*geomap)[i], (*geomap)[j], *dataset));
        }
      }
      // find the R smallest distances
      SmallBoundedPriorityQueue<disttime_t> sortedCentersDistances(breitling_constraints::MANDATORY_REGION_COUNT);
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
      SmallBoundedPriorityQueue<disttime_t> sortedDistances(breitling_constraints::MINIMUM_STATION_COUNT);
      for (stationidx_t s1 = 1; s1 < stationCount; s1++)
        for (stationidx_t s2 = 0; s2 < s1; s2++)
          sortedDistances.insert(m_adjencyMatrix.distanceUncached(s1, s2));
      m_minDistancePerRemainingStationCount[0] = 0;
      for (region_t r = 0; r < breitling_constraints::MINIMUM_STATION_COUNT; r++) {
        m_minDistancePerRemainingStationCount[1 + r] = m_minDistancePerRemainingStationCount[r] + sortedDistances.top();
        sortedDistances.pop();
      }
    }
  }

private:
  disttime_t lowerBound(const Label &label)
  {
    unsigned char regionCountLeftToExplore = breitling_constraints::MANDATORY_REGION_COUNT - utils::countRegions(label.visitedRegions);
    disttime_t minDistanceForRegions = m_minDistancePerRemainingRegionCount[regionCountLeftToExplore];
    unsigned char stationsLeftToExplore = breitling_constraints::MINIMUM_STATION_COUNT - label.visitedStationCount;
    disttime_t minDistanceForStations = m_minDistancePerRemainingStationCount[stationsLeftToExplore];
    disttime_t distanceToTarget = m_adjencyMatrix.distanceToTargetStation(label.currentStation);
    return std::max({ minDistanceForRegions, distanceToTarget, minDistanceForStations });
  }

  // Labels with high scores will be explored first
  score_t scoreLabel(const Label &label)
  {
#if 0
    // a label is good if...
    float score = 0;
    // it visited a lot of stations
    score += label.visitedStationCount * .1f;
    // it visited an appropriate number of regions
    // where "appropriate" means having explored regions at the same rate as explored stations
    score -= std::max(std::abs((float)utils::countRegions(label.visitedRegions) / breitling_constraints::MANDATORY_REGION_COUNT - (float)label.visitedStationCount / breitling_constraints::MINIMUM_STATION_COUNT) - .25f, 0.f);
    // it has fuel, but not too much
    score += label.currentFuel * .1f; // TODO score higher whenever (fuel is high)==(night is soon)
    // it is quick
    score -= label.currentTime * .3f;
    // there should more be factors here, the distance between stations and the plane speed/fuel capacity is ignored
    // this attempt at scoring was discarded because it was way to expensive to compute
    return score;
#elif 1
    float score = 0;
    score += label.visitedStationCount;
    score -= label.currentTime * .3f;
    if (m_bestTime != m_noBestTime)
      score += (float)rand()/RAND_MAX*3.f;
    return score;
#elif 1
    float score = 0;
    score += label.visitedStationCount;
    score -= label.currentTime * .3f;
    return score;
#else
    return (float)rand() / RAND_MAX;
#endif
  }

  inline void tryExplore(const Label &source, std::vector<Label> &explorationLabels, stationidx_t nextStationIdx, disttime_t distanceToNext)
  {
    region_t currentExtendedRegion = m_stationExtendedRegions[source.currentStation];
    size_t currentVisitedRegionCount = utils::countRegions(source.visitedRegions);
    const ProblemStation &nextStation = (*m_geomap)[nextStationIdx];
    region_t newLabelVisitedRegions = source.visitedRegions | m_stationRegions[nextStationIdx];
    region_t newLabelExtendedRegion = m_stationExtendedRegions[nextStationIdx];

    if (source.visitedStations.isSet(nextStationIdx))
      return; // station already visited
    if (distanceToNext > source.currentFuel)
      return; // not enough fuel
    if (source.currentTime + distanceToNext > m_bestTime)
      return; // already dominated on time
    if (breitling_constraints::MINIMUM_STATION_COUNT - source.visitedStationCount < breitling_constraints::MANDATORY_REGION_COUNT - utils::countRegions(newLabelVisitedRegions))
      return; // 3 regions left to visit but only 2 more stations to go through
    if (!nextStation.canBeUsedToFuel() && source.currentFuel - distanceToNext < m_adjencyMatrix.distanceToNearestStationWithFuel(nextStationIdx))
      return; // 1 hop is possible, 2 are not because of low fuel
    if (!nextStation.isAccessibleAtNight() && (nextStationIdx != m_dataset->targetStation) && utils::isTimeInNightPeriod(source.currentTime + distanceToNext, *m_dataset))
      return; // the station is not accessible during the night
    if ((currentExtendedRegion & ~source.visitedRegions) && newLabelExtendedRegion != currentExtendedRegion)
      return; // Ir strategy: the current region is not explored and the new label is going away
    if (currentVisitedRegionCount != breitling_constraints::MANDATORY_REGION_COUNT &&
        (currentExtendedRegion & source.visitedRegions) &&
        (newLabelExtendedRegion != currentExtendedRegion) &&
        (source.visitedRegions & newLabelExtendedRegion))
      return; // Ir strategy: the current region is explored and the label is going in an already visited extended region, does not apply if all regions are already visited

    bool shouldExploreNoRefuel = true;
    bool shouldExploreWithRefuel = nextStation.canBeUsedToFuel();

    if (m_dataset->timeToRefuel == 0 && nextStation.canBeUsedToFuel()) // if the time to refuel is 0 refuel every time it is possible
      shouldExploreNoRefuel = false;

    if (!shouldExploreNoRefuel && !shouldExploreWithRefuel)
      return;

    if (shouldExploreNoRefuel) { // without refuel
      Label &explorationLabel = explorationLabels.emplace_back(source); // copy the source
      explorationLabel.currentFuel -= distanceToNext;
      explorationLabel.currentTime += distanceToNext;
      explorationLabel.visitedRegions = newLabelVisitedRegions;
      explorationLabel.visitedStationCount++;
      explorationLabel.visitedStations.setSet(nextStationIdx);
      explorationLabel.currentStation = nextStationIdx;
      explorationLabel.score = scoreLabel(explorationLabel);
      assert(explorationLabel.score > Label::MIN_POSSIBLE_SCORE);
    }

    if (shouldExploreWithRefuel) { // with refuel
      Label &explorationLabel = explorationLabels.emplace_back(source);
      explorationLabel.currentFuel = utils::getPlaneAutonomy(*m_dataset);
      explorationLabel.currentTime += distanceToNext + m_dataset->timeToRefuel;
      explorationLabel.visitedRegions = newLabelVisitedRegions;
      explorationLabel.visitedStationCount++;
      explorationLabel.visitedStations.setSet(nextStationIdx);
      explorationLabel.currentStation = nextStationIdx;
      explorationLabel.score = scoreLabel(explorationLabel);
      assert(explorationLabel.score > Label::MIN_POSSIBLE_SCORE);
    }
  }

  inline void explore(const Label &source, std::vector<Label> &explorationLabels)
  {
    if (source.visitedStationCount == breitling_constraints::MINIMUM_STATION_COUNT - 1 && m_dataset->targetStation != BreitlingData::NO_TARGET_STATION) {
      // only 1 station left, only try to reach the target station
      stationidx_t nextStationIdx = m_dataset->targetStation;
      disttime_t distanceToNext = m_adjencyMatrix.distanceToTargetStation(source.currentStation);
      tryExplore(source, explorationLabels, nextStationIdx, distanceToNext);
    } else {
      // try to reach any station that is close enough
      for (size_t i = 0; i < m_adjencyMatrix.adjencyCount(source.currentStation); i++) {
        stationidx_t nextStationIdx = m_adjencyMatrix.getAdjencyNextStation(source.currentStation, i);
        disttime_t distanceToNext = m_adjencyMatrix.getAdjencyNextDistance(source.currentStation, i);
        tryExplore(source, explorationLabels, nextStationIdx, distanceToNext);
      }
    }
  }

  inline bool dominates(const Label &dominating, const Label &dominated)
  {
    // there is no need to check dominating.currentStation==dominated.currentStation as
    // an index on currentStation is used to order labels
#if 0
    return
      // the dominating label visited at least the stations visited by the dominated label
      // no need to check visited station/region counts as all stations visited by the
      // dominated label are also visited by the dominating label
      dominating.visitedStations.contains(dominated.visitedStations)
      //&& dominating.currentFuel >= dominated.currentFuel // the dominating has at lest as much fuel
      && dominating.currentTime <= dominated.currentTime // the dominating takes less time
      ;
#else
    // more lenient domination check, this one ensure that only 100 labels exist at most
    // per station and ~per visited region combination~, retaining only the one that spent
    // the less time to get there
    return
      dominating.visitedStationCount == dominated.visitedStationCount
      && (dominated.visitedRegions & ~dominating.visitedRegions) == 0
      && dominating.currentTime <= dominated.currentTime
      ;
#endif
  }

  ProblemPath reconstitutePath(fragmentidx_t endFragment)
  {
    ProblemPath path;
    while (true) {
      const PathFragment &fragment = m_fragments[endFragment];
      path.push_back((*m_geomap)[fragment.getStationIdx()]);
      fragmentidx_t parent = fragment.getPreviousFragment();
      if (parent == endFragment)
        break; // end of the path reached
      endFragment = parent;
    }
    std::reverse(path.begin(), path.end());
    assert(path.size() == breitling_constraints::MINIMUM_STATION_COUNT);
    return path;
  }

public:
  ProblemPath labelSetting(bool *stopFlag)
  {
    fragmentidx_t bestPath = Label::NO_FRAGMENT;

    std::vector<Label> explorationLabels;
    explorationLabels.reserve(100);

#ifdef USE_HEURISTIC_LOWER_BOUND
    // quickly find an upper bound
    Path heuristicPath = NaturalBreitlingSolver(*m_dataset).solveForPath(*m_geomap);
    noBestTime = bestTime = m_dataset->departureTime + utils::realDistanceToTimeDistance(heuristicPath.length(), *m_dataset);
#endif

    { // create the initial label
      Label initialLabel{};
      initialLabel.currentFuel = utils::getPlaneAutonomy(*m_dataset);
      initialLabel.currentTime = m_dataset->departureTime;
      initialLabel.currentStation = m_dataset->departureStation;
      initialLabel.visitedStations.setSet(initialLabel.currentStation);
      initialLabel.visitedRegions = m_stationRegions[initialLabel.currentStation];
      initialLabel.visitedStationCount = 1;
      initialLabel.score = 0.f; // score does not matter, the initial label will be explored first
      initialLabel.pathFragment = m_fragments.pushInitial(initialLabel.currentStation);
      labelidx_t initialIndex = m_labels.push(initialLabel);
      m_labelsPerStationsIndex[initialLabel.currentStation].push_back(initialIndex);
      m_bestLabelsQueue.tryInsertInQueue(initialIndex);
    }

    size_t iteration = 0;
    size_t solutionsFound = 0;
    long long searchBeginTime = utils::currentTimeMs();

    // loop until we forcibly stop the algorithm, a second stopping condition is in the loop
    while (stopFlag==nullptr || !*stopFlag) {
      iteration++;
      // take the best label currently yet-to-be-explored
      labelidx_t exploredIndex = m_bestLabelsQueue.popFront();
      if (exploredIndex == -1) {
        // no more labels available, exit the loop and end the algorithm
        break;
      }

      Label &explored = m_labels[exploredIndex];
      explored.setExplored();

      // the lower bound may have been lowered since the label was added, if
      // it is no longer of intereset discard it before doing any exploration
      if (m_bestTime != m_noBestTime && lowerBound(explored) > m_bestTime)
        continue;

      // explore the current label to discover new possible *and interesting* paths
      explore(explored, explorationLabels);
      // there cannot be too many children for a single label, otherwise there will be overflow in the PathFragment structure
      assert(explorationLabels.size() <= PathFragment::MAX_CHILDREN_COUNT);

      for (Label &nextLabel : explorationLabels) {
        if (nextLabel.visitedStationCount == breitling_constraints::MINIMUM_STATION_COUNT) {
          if (nextLabel.currentTime < m_bestTime && utils::countRegions(nextLabel.visitedRegions) == breitling_constraints::MANDATORY_REGION_COUNT) {
            // path is complete and better than the best one found so far
            if (m_bestTime != m_noBestTime)
              m_fragments.release(bestPath);
            // nextLabel had its parent's pathFragment until now
            nextLabel.pathFragment = m_fragments.push(nextLabel.currentStation, nextLabel.pathFragment);
            // record new best
            bestPath = nextLabel.pathFragment;
            m_bestTime = nextLabel.currentTime;
            std::cout << "improved " << (utils::currentTimeMs() - searchBeginTime)/1000.f << " " << (m_bestTime - m_dataset->departureTime) << std::endl;
            //writeStations(nextLabel.visitedStations, lastStation, *m_geomap, "out_"+std::to_string(solutionsFound++)+".svg");
          }
        } else {
          // remove dominated labels
          // this assumes explore() did not produce two labels with one dominating the other
          std::vector<labelidx_t> &otherLabelsAtSameStation = m_labelsPerStationsIndex[nextLabel.currentStation];
          auto it = otherLabelsAtSameStation.begin();
          bool nextIsDominated = false;
          while (it != otherLabelsAtSameStation.end()) {
            Label &otherLabel = m_labels[*it];
            if (dominates(nextLabel, otherLabel)) {
              // release the dominated label
              m_bestLabelsQueue.remove(*it);
              m_fragments.release(m_labels[*it].pathFragment);
              m_labels.free(*it);
              it = otherLabelsAtSameStation.erase(it);
            } else if (dominates(otherLabel, nextLabel)) {
              nextIsDominated = true;
              break;
            } else {
              it++;
            }
          }
          if (!nextIsDominated) {
            // create a fragment, was not done before because labels that are immediately discarded
            // do not need to create fragments
            nextLabel.pathFragment = m_fragments.push(nextLabel.currentStation, nextLabel.pathFragment);
            // append the new label to the open list
            labelidx_t nextLabelIndex = m_labels.push(nextLabel); // may invalidate &explored, and &nextLabel cannot be written to anymore
            m_labelsPerStationsIndex[nextLabel.currentStation].push_back(nextLabelIndex);
            m_bestLabelsQueue.tryInsertInQueue(nextLabelIndex);
            PROFILING_COUNTER_INC(discovered_label);
          }
        }
      }

      //m_fragments.release(m_labels[exploredIndex].pathFragment); // TODO free fragments of labels with no children that are still in m_labels because they can still dominate other labels, but do not free fragments of labels that got dominated but had their fragments already freed

      explorationLabels.clear();

      PROFILING_COUNTER_INC(label_explored);

#ifdef LIMITED_SEARCH_TIME
      if (utils::currentTimeMs() - searchBeginTime > LIMITED_SEARCH_TIME*1000) {
        std::cout << "Stopped searching after " << LIMITED_SEARCH_TIME << "s" << std::endl;
        break;
      }
#endif
    }

    if (m_bestTime != m_noBestTime) {
      disttime_t finalTime = m_bestTime - m_dataset->departureTime;
      std::cout << "Found with time=" << finalTime << " distance=" << finalTime*m_dataset->planeSpeed << std::endl;
      // a path was found, but is was not stored *as a path* but as a
      // collection of path fragments so we must reconstitute it first
      return reconstitutePath(bestPath);
    } else {
      // did not find a valid path
#ifdef USE_HEURISTIC_LOWER_BOUND
      std::cout << "No path found, falling back to heuristic" << std::endl;
      return heuristicPath;
#else
      std::cout << "No path found" << std::endl;
      return {};
#endif
    }
  }
};

ProblemPath LabelSettingBreitlingSolver::solveForPath(const ProblemMap &map, bool *stopFlag)
{
  // prepare the dataset & geomap, that means having the departure 
  // station at index 0 in the geomap, the target station at index N-1
  //m_dataset.targetStation = BreitlingData::NO_TARGET_STATION;
  LabelSetting labelSetting{ &map, &m_dataset };
  return labelSetting.labelSetting(stopFlag);
}
