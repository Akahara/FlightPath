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
 * S = {initial label}
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
 *        remove L" from S and from R it is was there
 *      if L' is acceptable
 *        if time(L') < besttime
 *          besttime = time(L')
 *          bestlabel = L'
 *      else
 *        add L' to S and R
 * return bestlabel
 */



//#define DEBUG_PRINT(x) std::cout << x << "\n"
#define DEBUG_PRINT(x)

#define TO_REMOVE(x) x

#ifdef _DEBUG

#define PROFILING_COUNTER_DEF(name) \
  static size_t __counter_##name = 0;\
  static unsigned char __counterinitializer_##name = ([]() {\
    profiling_stats::counters.insert({ #name, &__counter_##name });\
    return 0;\
  })();\
  __counter_##name
#define PROFILING_COUNTER_ADD(name, count) { PROFILING_COUNTER_DEF(name) += count; }
#define PROFILING_COUNTER_INC(name) { PROFILING_COUNTER_DEF(name)++; }

namespace profiling_stats {

static std::unordered_map<const char *, size_t*> counters;

}
#else
#define PROFILING_COUNTER_DEF(name)
#define PROFILING_COUNTER_ADD(name, count)
#define PROFILING_COUNTER_INC(name)
#endif

namespace packed_data_structure {

constexpr size_t MAX_SUPPORTED_STATIONS = 512;
constexpr size_t BITS_PER_STATION_IDX = std::bit_width(MAX_SUPPORTED_STATIONS - 1);
constexpr size_t MAX_CONCURENT_LABELS = 4096;
constexpr size_t BITS_PER_LABEL_IDX = std::bit_width(MAX_CONCURENT_LABELS - 1);
constexpr size_t MAX_REGION_COUNT = breitling_constraints::MANDATORY_REGION_COUNT;
constexpr size_t BITS_PER_REGION_SET = MAX_REGION_COUNT;
constexpr size_t BITS_FOR_VISITED_STATION_COUNT = std::bit_width(breitling_constraints::MINIMUM_STATION_COUNT - 1);

}

// TODO organize classes better

typedef uint16_t stationidx_t;  // index in the Geomap
typedef uint32_t labelidx_t;    // index in the LabelsArena
typedef float disttime_t;       // a duration (unit is defined by the user)
typedef disttime_t distance_t;  // a distance, see the comment on time/distance relation
typedef uint8_t region_t;       // bit field, 0b1010 means that the 2nd and 4th regions have been visited
typedef uint8_t regionidx_t;    // offset in a region_t, ie. regionidx_t=2 means the third region and corresponds to region_t=0b100
typedef float score_t;
typedef uint32_t fragmentidx_t; // TODO comment
//typedef std::bitset<packed_data_structure::MAX_SUPPORTED_STATIONS> stationset_t; // TODO comment

// make sure that all regions combinations can be represented with region_t
// TODO change assertions after having optimized the Label struct
static_assert(breitling_constraints::MANDATORY_REGION_COUNT < sizeof(region_t) * CHAR_BIT);

class StationSet {
private:
  using word_t = unsigned long long;
  static constexpr size_t WORD_SIZE = sizeof(word_t) * CHAR_BIT;
  static constexpr size_t WORD_COUNT = packed_data_structure::MAX_SUPPORTED_STATIONS / WORD_SIZE;
  word_t m_array[WORD_COUNT]{};

public:
  bool isSet(size_t stationIdx) const
  {
    return m_array[stationIdx / WORD_SIZE] & ( 1ll << (stationIdx & (WORD_SIZE - 1)));
  }

  void setSet(size_t stationIdx)
  {
    m_array[stationIdx / WORD_SIZE] |= 1ll << (stationIdx & (WORD_SIZE - 1));
  }

  bool contains(const StationSet &other) const
  {
    for (size_t i = 0; i < WORD_COUNT; i++)
      if (other.m_array[i] & ~m_array[i])
        return false;
    return true;
  }
  
};

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

template<typename T>
constexpr unsigned char first1bitIndex(T x)
{
  if (x == 0)
    return -1;
  for (unsigned char i = 0; i < sizeof(T) * CHAR_BIT; i++) {
    if (x & (1 << i))
      return i;
  }
  return -1; // unreachable
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

template<size_t S>
constexpr std::array<unsigned char, S> createFirstBitLookupTable()
{
  std::array<unsigned char, S> lookupTable;
  for (size_t i = 0; i < S; i++)
    lookupTable[i] = first1bitIndex(i);
  return lookupTable;
}

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
static uint8_t firstRegionSetIndex(region_t regionBitField)
{
  return FIRST_BIT_INDEX_LOOKUP_TABLE[regionBitField];
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

static inline disttime_t planeTimeFuelCapacity(const BreitlingData &dataset)
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
private:
  static constexpr fragmentidx_t PREVIOUS_FRAGMENT_IDX_EMPTY_MARKER = -1;

  uint16_t packedStationUseCount; // 9 bits for the station (up to 512) and 7 bits for the use count (up to 127 immediate children + 1 immediate use)
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
  
  StationSet visitedStations;
  stationidx_t currentStation : packed_data_structure::BITS_PER_STATION_IDX;
  region_t visitedRegions : packed_data_structure::BITS_PER_REGION_SET;
  uint8_t visitedStationCount : packed_data_structure::BITS_FOR_VISITED_STATION_COUNT;
  disttime_t currentTime; // could be an unsigned int, with less than 32 bits even
  disttime_t currentFuel; // could be an unsigned int, with less than 32 bits even
  score_t score;          // used to explore best labels first
  fragmentidx_t pathFragment = NO_FRAGMENT;

  // arbitrary floats
  // TODO comment
  static constexpr score_t MIN_POSSIBLE_SCORE = std::numeric_limits<float>::lowest() * .5f;
  static constexpr score_t EMPTY_SCORE_MARKER = std::numeric_limits<float>::lowest() * .95f;
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
  distance_t distance;
  stationidx_t station;
};

struct compareLimitedAdjency {
  bool operator()(const LimitedAdjency &l1, const LimitedAdjency &l2) { return l1.distance < l2.distance; }
};

struct LabelRef {
  labelidx_t labelIndex;
  float      labelScore;
};

template<class T, typename Compare = std::less<T>>
class SmallBoundedPriorityQueue {
private:
  std::vector<T> m_queue;
public:
  SmallBoundedPriorityQueue(size_t capacity)
  {
    m_queue.reserve(capacity);
  }

  const T &top()
  {
    return m_queue[0];
  }

  void pop()
  {
    assert(m_queue.size() > 0);
    m_queue.erase(m_queue.begin());
  }

  template<class K>
  void transferTo(K &k)
  {
    size_t capacity = m_queue.capacity();
    k = std::move(m_queue);
    m_queue.reserve(capacity);
  }

  void insert(T x)
  {
    for (size_t i = 0; i < m_queue.size(); i++) {
      if (Compare{}(x, m_queue[i])) {
        if (m_queue.size() >= m_queue.capacity())
          m_queue.pop_back();
        m_queue.insert(m_queue.begin() + i, std::move(x));
        return;
      }
    }
    if(m_queue.size() != m_queue.capacity())
      m_queue.push_back(std::move(x));
  }
};

template<class S>
struct leftComp {
  constexpr bool operator()(const S &, const S &)
  {
    return true;
  }
};

template<class S>
using SmallBoundedQueue = SmallBoundedPriorityQueue<S, leftComp<S>>;

template<typename T, typename S>
concept InlineAllocator = requires(const T &constAllocator, T &allocator, S &slot)
{
  { constAllocator.isFree(slot) } -> std::same_as<bool>;
  { allocator.setFree(&slot) };
  // TODO add allocator.setFree(&fromSlot, &toSlot);
};

/*
 * An inline allocator is a structure that is able to differenciate
 * allocated and non-allocated memory for a specific structure.
 * It does not do proper allocation per-say but it helps allocators
 * that have no information of (non)allocated memory, like the Clock
 * Arena Allocator.
 */
template<class S>
struct DefaultInlineAllocator {
  inline bool isFree(const S &slot) const
  {
    return *(unsigned char *)&slot == 0xff;
  }

  inline void setFree(S *slot)
  {
    memset(slot, 0xff, sizeof(S));
  }
};

static_assert(InlineAllocator<DefaultInlineAllocator<Label>,Label>);

template<class S, typename index_t = size_t, InlineAllocator<S> InlineAllocator = DefaultInlineAllocator<S>>
class ClockArenaAllocator {
protected:
TO_REMOVE(public:)
  static constexpr float FREE_SLOTS_THRESHOLD = .05f; // if there are less than 5% free slots left realloc into a bigger array

  InlineAllocator m_allocator;
  S      *m_array;
  index_t m_next;
  size_t  m_size;
  size_t  m_freeSlotCount;

public:
  explicit ClockArenaAllocator(size_t size)
    : m_size(size), m_next(0), m_freeSlotCount(size)
  {
    m_array = (S *)malloc(sizeof(S) * size);
    if (!m_array) throw std::runtime_error("Out of memory on first allocation");
    for (size_t i = 0; i < size; i++)
      m_allocator.setFree(&m_array[i]);
  }

  ~ClockArenaAllocator()
  {
    std::free(m_array);
  }

  ClockArenaAllocator(const ClockArenaAllocator &) = delete;
  ClockArenaAllocator &operator=(const ClockArenaAllocator &) = delete;

  S &operator[](index_t idx) { assert(idx < m_size); return m_array[idx]; }

  index_t alloc()
  {
    if (m_freeSlotCount > 0) {
      // if there are free slots left, find one in next..size
      for (; m_next < m_size; m_next++) {
        if (m_allocator.isFree(m_array[m_next])) {
          m_freeSlotCount--;
          assert(m_allocator.isFree(m_array[m_next]));
          return m_next++;
        }
      }
    } else {
      // if there are none skip the search
      m_next = m_size;
    }

    // end of the buffer reached, if there are too few free slots left realloc into a bigger array
    if (m_freeSlotCount < m_size * FREE_SLOTS_THRESHOLD || m_freeSlotCount == -1) {
      // no more space, realloc the array
      size_t newSize = (size_t)(m_size * 1.5f);
      if (newSize > std::numeric_limits<index_t>::max())
        throw std::runtime_error("Allocated too many objects");
      S *newArray = (S *)realloc(m_array, newSize*sizeof(S));
      if (!newArray)
        throw std::runtime_error("Out of memory");
      for (size_t i = m_size; i < newSize; i++)
        m_allocator.setFree(&newArray[i]);
      m_freeSlotCount += newSize - m_size;
      m_array = newArray;
      m_size = newSize;
      m_freeSlotCount--;
      assert(m_allocator.isFree(m_array[m_next]));
      return m_next++;
    }

    // next..size was full but the buffer was not reallocated, that means there are many slots empty in 0..next
    // (the loop loops until m_size but a free slot will be found before)
    for (m_next = 0; m_next < m_size; m_next++) {
      if (m_allocator.isFree(m_array[m_next])) {
        m_freeSlotCount--;
        assert(m_allocator.isFree(m_array[m_next]));
        return m_next++;
      }
    }

    assert(false); // unreachable
    return -1;
  }

  void free(index_t slotIdx)
  {
    assert(!m_allocator.isFree(m_array[slotIdx]));
    m_freeSlotCount++;
    m_allocator.setFree(&m_array[slotIdx]);
  }

  inline index_t getMaxIndex() const
  {
    return m_size;
  }

  inline index_t nextValidIndex(index_t currentIndex) const
  {
    do {
      currentIndex++;
    } while (currentIndex < m_size && m_allocator.isFree(m_array[currentIndex]));
    return currentIndex;
  }

  inline index_t firstValidIndex() const
  {
    return nextValidIndex(-1);
  }
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
};

class LabelsArena : TO_REMOVE(public) ClockArenaAllocator<Label, labelidx_t, LabelInlineAllocator> {
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

class BestLabelsQueue {
private:
  std::vector<LabelRef> m_bestLabels; // TODO using a list here would be better
  score_t               m_minBestScore;
  const LabelsArena    *m_labelsArena;
public:
  BestLabelsQueue(const LabelsArena *labelsArena, size_t cacheSize)
    : m_labelsArena(labelsArena), m_minBestScore(Label::MIN_POSSIBLE_SCORE)
  {
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
    float score = m_labelsArena->m_array[labelIndex].score;
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

class PartialAdjencyMatrix {
private:
TO_REMOVE(public:)
  std::vector<std::vector<LimitedAdjency>> m_adjencyMatrix; // NxM matrix M<<N
  std::vector<distance_t> m_distanceToTarget; // 1xN matrix, the distances to the target station must be kept somehow
  std::vector<distance_t> m_distanceToNearestRefuel;
  const ProblemMap *m_geomap;
  const BreitlingData *m_dataset;

public:
  PartialAdjencyMatrix(const ProblemMap *geomap, const BreitlingData *dataset)
    : m_adjencyMatrix(geomap->size()),
    m_distanceToTarget(geomap->size()),
    m_distanceToNearestRefuel(geomap->size()),
    m_geomap(geomap),
    m_dataset(dataset)
  {
    //SmallBoundedQueue<LimitedAdjency> nearestStationsQueue(geomap->size() / 4); // keep 1/4th of the available links
    SmallBoundedPriorityQueue<LimitedAdjency, compareLimitedAdjency> nearestStationsQueue(20); // keep only the 20 nearest links per station

    for (stationidx_t i = 0; i < geomap->size()-1; i++) {
      distance_t minDistanceToFuel = std::numeric_limits<distance_t>::max();
      stationidx_t nearestStationWithFuel = -1;
      for (stationidx_t j = 0; j < geomap->size()-1; j++) {
        if (i == j)
          continue;
        distance_t distance = utils::timeDistanceBetweenStations((*geomap)[i], (*geomap)[j], *dataset);
        if (i == geomap->size() - 1)
          m_distanceToTarget[j] = distance;
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

  inline distance_t getAdjencyNextDistance(stationidx_t currentStation, size_t idx)
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
    : m_size(size), m_array(new T[size * (size - 1) / 2]())
  {
  }

  ~TriangularMatrix()
  {
    delete[] m_array;
  }

  void fill(const T &value)
  {
    for (size_t i = 0; i < m_size * (m_size - 1) / 2; i++)
      m_array[i] = value;
    //std::fill(m_array, m_size * (m_size - 1) / 2, value);
  }

  T &at(size_t i, size_t j)
  {
    assert(i != j);
    assert(i < m_size && j < m_size);
    if (i > j) std::swap(i, j);
    size_t idx = m_size * (m_size - 1) / 2 - (m_size - i) * (m_size - i - 1) / 2 + j - i - 1;
    return m_array[idx];
  }
};

#if USE_FULL_ADJENCY_MATRIX
typedef FullAdjencyMatrix AdjencyMatrix; // maybe not completely up to date
#else
typedef PartialAdjencyMatrix AdjencyMatrix;
#endif

#if 1
static void writeDistanceMatrix(PartialAdjencyMatrix &matrix)
{
  std::ofstream file{ "debug_distancematrix.svg" };

  double
    minLon = std::numeric_limits<double>::max(),
    minLat = std::numeric_limits<double>::max(),
    maxLon = std::numeric_limits<double>::min(),
    maxLat = std::numeric_limits<double>::min();
  for (const ProblemStation &station : *matrix.m_geomap) {
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

  for (const ProblemStation &station : *matrix.m_geomap) {
    float red = station.canBeUsedToFuel() ? 1 : 0;
    float green = .5f;
    float blue = station.isAccessibleAtNight() ? 1 : 0;
    file
      << "<circle cx=\"" << station.getLocation().lon << "\" cy=\"" << station.getLocation().lat << "\" r=\".1\" "
      << "fill=\"rgb(" << red * 255 << "," << green * 255 << "," << blue * 255 << ")\"/>\n";
  }

  for (size_t i = 0; i < matrix.m_geomap->size(); i++) {
    const ProblemStation &s1 = (*matrix.m_geomap)[i];
    int r = rand();
    for (size_t j = 0; j < matrix.adjencyCount(i); j++) {
      stationidx_t n = matrix.getAdjencyNextStation(i, j);
      const ProblemStation &s2 = (*matrix.m_geomap)[n];
      file << "<line x1=\"" << s1.getLocation().lon << "\" y1=\"" << s1.getLocation().lat << "\" "
        << "x2=\"" << s2.getLocation().lon << "\" y2=\"" << s2.getLocation().lat << "\" "
        << "stroke-width=\".003\" stroke=\"#" << std::hex << r%0xffffff << std::dec << "\" />\n";
    }
  }
  file << "</svg>" << std::endl;
}

static void writeRegions(const std::vector<region_t> &regions, const std::vector<region_t> &extendedRegions, const ProblemMap &map)
{
  std::ofstream file{ "debug_regions.svg" };

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

  const char *colors[4] = { "red","blue","green","purple" };

  for(stationidx_t i = 0; i < map.size(); i++) {
    const Location &loc = map[i].getLocation();
    regionidx_t rprimary = utils::firstRegionSetIndex(regions[i]);
    regionidx_t rextended = utils::firstRegionSetIndex(extendedRegions[i]);
    file
      << "<circle cx=\"" << loc.lon << "\" cy=\"" << loc.lat << "\" r=\".15\" "
      << "fill=\"" << (regions[i] ? colors[rprimary] : "black") << "\"/>\n";
    file
      << "<circle cx=\"" << loc.lon << "\" cy=\"" << loc.lat << "\" r=\".1\" "
      << "fill=\"" << (extendedRegions[i] ? colors[rextended] : "black") << "\"/>\n";
  }

  file << "</svg>" << std::endl;
}
#endif


class LabelSetting {
private:
  static constexpr region_t NO_REGION = 0;

  const ProblemMap     *m_geomap;
  const BreitlingData  *m_dataset;

  AdjencyMatrix         m_adjencyMatrix;
  std::vector<region_t> m_stationRegions;
  std::vector<region_t> m_stationExtendedRegions;

  distance_t            m_minDistancePerRemainingRegionCount[breitling_constraints::MANDATORY_REGION_COUNT + 1];
  distance_t            m_minDistancePerRemainingStationCount[breitling_constraints::MINIMUM_STATION_COUNT + 1];

  LabelsArena           m_labels = LabelsArena(20'000); // start with min. 20k labels, it will surely grow during execution
  PathFragmentsArena    m_fragments = PathFragmentsArena(20'000);
  BestLabelsQueue       m_bestLabelsQueue;
  std::vector<std::vector<labelidx_t>> m_labelsPerStationsIndex;

public:
  LabelSetting(const ProblemMap *geomap, const BreitlingData *dataset)
    : m_geomap(geomap),
    m_adjencyMatrix(geomap, dataset),
    m_stationRegions(geomap->size(), NO_REGION),
    m_stationExtendedRegions(geomap->size(), NO_REGION),
    m_dataset(dataset),
    m_labelsPerStationsIndex(geomap->size()),
    m_bestLabelsQueue(&m_labels, 20)
  {
    size_t stationCount = geomap->size();
    constexpr size_t regionCount = breitling_constraints::MANDATORY_REGION_COUNT;

    // check that the dataset is prepared
    assert(stationCount > 0);
    assert(dataset->departureStation == 0);
    assert(dataset->targetStation == geomap->size() - 1);
    if (stationCount > std::numeric_limits<stationidx_t>::max())
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
        distance_t minDist = std::numeric_limits<distance_t>::max();
        regionidx_t extendedRegion = -1;
        const ProblemStation &station = (*geomap)[i];
        for (regionidx_t r = 0; r < regionCount; r++) {
          distance_t dist = geometry::distance(regionsCenters[r].location, station.getLocation());
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
      TriangularMatrix<distance_t> regionAdjencyMatrix{ regionCount };
      regionAdjencyMatrix.fill(std::numeric_limits<distance_t>::max());
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
          distance_t &distance = regionAdjencyMatrix.at(riidx, rjidx);
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
      SmallBoundedPriorityQueue<distance_t> sortedDistances(breitling_constraints::MINIMUM_STATION_COUNT);
      for (stationidx_t s1 = 1; s1 < stationCount; s1++)
        for (stationidx_t s2 = 0; s2 < s1; s2++)
          sortedDistances.insert(m_adjencyMatrix.distanceUncached(s1, s2));
      m_minDistancePerRemainingStationCount[0] = 0;
      for (region_t r = 0; r < breitling_constraints::MINIMUM_STATION_COUNT; r++) {
        m_minDistancePerRemainingStationCount[1 + r] = m_minDistancePerRemainingStationCount[r] + sortedDistances.top();
        sortedDistances.pop();
      }
    }

    writeDistanceMatrix(m_adjencyMatrix);
    writeRegions(m_stationRegions, m_stationExtendedRegions, *m_geomap);
  }

private:
  disttime_t lowerBound(const Label &label)
  {
    unsigned char regionCountLeftToExplore = breitling_constraints::MANDATORY_REGION_COUNT - utils::countRegions(label.visitedRegions);
    distance_t minDistanceForRegions = m_minDistancePerRemainingRegionCount[regionCountLeftToExplore];
    unsigned char stationsLeftToExplore = breitling_constraints::MINIMUM_STATION_COUNT - label.visitedStationCount;
    distance_t minDistanceForStations = m_minDistancePerRemainingStationCount[stationsLeftToExplore];
    distance_t distanceToTarget = m_adjencyMatrix.distanceToTargetStation(label.currentStation);
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
    // TODO there should be factors here, the distance between stations and the plane speed/fuel capacity is ignored
    return score;
#elif 0
    // currently there is no label domination implemented, the only reason
    // to explore one label before another is to lower the upper bound on
    // total time spent
    // to do that we score labels only depending on the number of stations
    // and regions they visited

    float score = 0;
    score += label.visitedStationCount;
    score += utils::countRegions(label.visitedRegions) * 20;
    score -= label.currentTime * .3f; // TODO find the right factor here
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

  inline void explore(const Label &source, disttime_t currentBestTime, std::vector<Label> &explorationLabels)
  {
    region_t currentExtendedRegion = m_stationExtendedRegions[source.currentStation];
    for (size_t i = 0; i < m_adjencyMatrix.adjencyCount(source.currentStation); i++) {
      stationidx_t nextStationIdx = m_adjencyMatrix.getAdjencyNextStation(source.currentStation, i);
      const ProblemStation &nextStation = (*m_geomap)[nextStationIdx];
      distance_t distanceToNext = m_adjencyMatrix.getAdjencyNextDistance(source.currentStation, i);
      region_t newLabelVisitedRegions = source.visitedRegions | m_stationRegions[nextStationIdx];
      region_t newLabelExtendedRegion = m_stationExtendedRegions[nextStationIdx];

      if (source.visitedStations.isSet(nextStationIdx))
        continue; // station already visited
      if (distanceToNext > source.currentFuel)
        continue; // not enough fuel
      if (breitling_constraints::MINIMUM_STATION_COUNT - source.visitedStationCount < breitling_constraints::MANDATORY_REGION_COUNT - utils::countRegions(newLabelVisitedRegions))
        continue; // 3 regions left to visit but only 2 more stations to go through
      if (!nextStation.canBeUsedToFuel() && source.currentFuel - distanceToNext < m_adjencyMatrix.distanceToNearestStationWithFuel(nextStationIdx))
        continue; // 1 hop is possible, 2 are not because of low fuel
      if (!nextStation.isAccessibleAtNight() && utils::isTimeInNightPeriod(source.currentTime + distanceToNext, *m_dataset))
        continue; // the station is not accessible during the night
      if ((currentExtendedRegion & ~source.visitedRegions) && newLabelExtendedRegion != currentExtendedRegion)
        continue; // Ir strategy: the current region is not explored and the new label is going away
      if ((currentExtendedRegion & source.visitedRegions) && (newLabelExtendedRegion != currentExtendedRegion) && (source.visitedRegions & newLabelExtendedRegion))
        continue; // Ir strategy: the current region is explored and the label is going in an already visited extended region

      bool shouldExploreNoRefuel = true;
      bool shouldExploreWithRefuel = nextStation.canBeUsedToFuel();

      if (m_dataset->timeToRefuel == 0 && nextStation.canBeUsedToFuel()) // if the time to refuel is 0 refuel every time it is possible
        shouldExploreNoRefuel = false;

      if (!shouldExploreNoRefuel && !shouldExploreWithRefuel)
        continue;

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
        explorationLabel.currentFuel = utils::planeTimeFuelCapacity(*m_dataset);
        explorationLabel.currentTime += distanceToNext + m_dataset->timeToRefuel;
        explorationLabel.visitedRegions = newLabelVisitedRegions;
        explorationLabel.visitedStationCount++;
        explorationLabel.visitedStations.setSet(nextStationIdx);
        explorationLabel.currentStation = nextStationIdx;
        explorationLabel.score = scoreLabel(explorationLabel);
        assert(explorationLabel.score > Label::MIN_POSSIBLE_SCORE);
      }
    }
  }

  inline bool dominates(const Label &dominating, const Label &dominated)
  {
    return
      dominating.visitedStations.contains(dominated.visitedStations) // the dominating label visited at least the stations visited by the dominated label
      //dominating.visitedStationCount >= dominated.visitedStationCount && (dominated.visitedRegions & ~dominating.visitedRegions) == 0
      && dominating.currentFuel >= dominated.currentFuel // the dominating has at lest as much fuel
      && dominating.currentTime <= dominated.currentTime // the dominating is at most as late
      //&& dominating.currentStation == dominated.currentStation // the two are at the same station // not necessary as an index on currentStation is used
      // no need to check visited station/region counts as all stations visited by the
      // dominated label are also visited by the dominating label
      ;
  }

  std::vector<ProblemStation> reconstitutePath(fragmentidx_t endFragment)
  {
    std::vector<ProblemStation> path;
    while (endFragment != PathFragment::NO_PARENT_FRAGMENT) {
      const PathFragment &fragment = m_fragments[endFragment];
      path.push_back((*m_geomap)[fragment.getStationIdx()]);
      endFragment = fragment.getPreviousFragment();
    }
    assert(path.size() == breitling_constraints::MINIMUM_STATION_COUNT);
    return path;
  }

public:
  std::vector<ProblemStation> labelSetting()
  {
    constexpr disttime_t noBestTime = std::numeric_limits<disttime_t>::max();
    disttime_t bestTime = noBestTime;
    fragmentidx_t bestPath = Label::NO_FRAGMENT;

    const stationidx_t lastStation = m_geomap->size() - 1;

    std::vector<Label> explorationLabels;
    explorationLabels.reserve(100);

    { // quickly find an upper bound
      bestTime = utils::realDistanceToTimeDistance(NaturalBreitlingSolver(*m_dataset).solveForPath(*m_geomap).length(), *m_dataset);
    }

    { // create the initial label
      Label initialLabel{};
      initialLabel.currentFuel = utils::planeTimeFuelCapacity(*m_dataset);
      initialLabel.currentTime = m_dataset->departureTime;
      initialLabel.currentStation = 0; // originate from station 0
      initialLabel.visitedStations.setSet(initialLabel.currentStation);
      initialLabel.visitedRegions = m_stationRegions[initialLabel.currentStation];
      initialLabel.visitedRegions = 0b1111; // FIX remove
      initialLabel.visitedStationCount = 1;
      initialLabel.score = 0.f; // score does not matter, the initial label will be explored first
      initialLabel.pathFragment = m_fragments.pushInitial(initialLabel.currentStation);
      labelidx_t initialIndex = m_labels.push(initialLabel);
      m_labelsPerStationsIndex[initialLabel.currentStation].push_back(initialIndex);
    }

    size_t iteration = 0;
    size_t maxDepth = 0;

    while (true) {
      iteration++;
      // take the best label currently yet-to-be-explored
      labelidx_t exploredIndex = m_bestLabelsQueue.popFront();
      if (exploredIndex == -1) {
        // no more labels available, exit the loop and end the algorithm
        break;
      }

      Label &explored = m_labels[exploredIndex];
      explored.setExplored();

      DEBUG_PRINT("peek " << exploredIndex);

      // the lower bound may have been lowered since the label was added, if
      // it is no longer of intereset discard it before doing any exploration
      if (bestTime != noBestTime && lowerBound(explored) > bestTime)
        continue;

      // explore the current label to discover new possible *and interesting* paths
      explore(explored, bestTime, explorationLabels);
      assert(explorationLabels.size() <= 127); // this is assumed by the PathFragment structure TODO use constants

      maxDepth = std::max(maxDepth, (size_t)explored.visitedStationCount);

      for (Label &nextLabel : explorationLabels) {
        assert(!nextLabel.visitedStations.isSet(lastStation));
        if (nextLabel.visitedStationCount == breitling_constraints::MINIMUM_STATION_COUNT - 1) {
          // only 1 station remaining, try to complete the path
          distance_t distanceToComplete = m_adjencyMatrix.distanceToTargetStation(nextLabel.currentStation);
          if (nextLabel.currentFuel > distanceToComplete &&
              nextLabel.currentTime + distanceToComplete < bestTime &&
              utils::countRegions(nextLabel.visitedRegions | m_stationRegions[lastStation]) == breitling_constraints::MANDATORY_REGION_COUNT) {
            // path is completeable and better than the best one found so far
            if (bestTime != noBestTime)
              m_fragments.release(bestPath);
            bestPath = m_fragments.push(lastStation, nextLabel.pathFragment);
            bestTime = nextLabel.currentTime + distanceToComplete;
            std::cout << "improved " << bestTime << std::endl;
          }
        } else {
          // remove dominated labels
          // this assumes explore() did not produce two labels with one dominating the other
          std::vector<labelidx_t> &otherLabelsAtSameStation = m_labelsPerStationsIndex[nextLabel.currentStation];
          auto it = otherLabelsAtSameStation.begin();
          bool nextIsDominated = false;
          while (it != otherLabelsAtSameStation.end()) {
            Label &otherLabel = m_labels[*it];
            if (dominates(otherLabel, nextLabel)) {
              nextIsDominated = true;
              break;
            } else if (dominates(nextLabel, otherLabel)) {
              // release the dominated label
              m_bestLabelsQueue.remove(*it);
              m_fragments.release(m_labels[*it].pathFragment);
              m_labels.free(*it);
              it = otherLabelsAtSameStation.erase(it);
            } else {
              it++;
            }
          }
          if (!nextIsDominated) {
            // create a fragment, was not done before because labels that are immediately discarded
            // do not need to create fragments
            nextLabel.pathFragment = m_fragments.push(nextLabel.currentStation, m_labels[exploredIndex].pathFragment);
            // append the new label to the open list
            labelidx_t nextLabelIndex = m_labels.push(nextLabel); // may invalidate &explored and &nextLabel cannot be written to anymore
            m_labelsPerStationsIndex[nextLabel.currentStation].push_back(nextLabelIndex);
            DEBUG_PRINT((int)nextLabel.currentStation << " <- " << (int)nextLabelIndex);
            PROFILING_COUNTER_INC(discovered_label);
          }
        }
      }

      //m_fragments.release(m_labels[exploredIndex].pathFragment); // TODO free fragments of labels with no children that are still in m_labels because they can still dominate other labels, but do not free fragments of labels that got dominated but had their fragments already freed

      explorationLabels.clear();
      //std::cout << (int)m_labels[exploredIndex].visitedStationCount << " ";
      if (iteration % 1000 == 0) {
        std::cout << maxDepth << std::endl;
      }

      PROFILING_COUNTER_INC(label_explored);
    }

    std::cout << "Found with time=" << bestTime << " distance=" << bestTime*m_dataset->planeSpeed << std::endl;

    if (bestTime != std::numeric_limits<disttime_t>::max()) {
      // a path was found, but is was not stored *as a path* but as a "visited station set"
      // so we must reconstitute it first
      return reconstitutePath(bestPath);
    } else {
      // did not find a valid path
      return {};
    }
  }
};

Path LabelSettingBreitlingSolver::solveForPath(const ProblemMap &map)
{
  // prepare the dataset & geomap, that means having the departure 
  // station at index 0 in the geomap, the target station at index N-1
  ProblemMap preparedMap = map;
  std::swap(preparedMap[0], preparedMap[m_dataset.departureStation]);
  std::swap(preparedMap[preparedMap.size() - 1], preparedMap[m_dataset.targetStation]);
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
