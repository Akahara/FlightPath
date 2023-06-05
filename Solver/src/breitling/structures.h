#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <chrono>

// ----------------------- Macros, debug/profiling utilities ----------------------
 
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

static std::unordered_map<const char *, size_t *> counters;

}

#else
#define PROFILING_COUNTER_DEF(name)
#define PROFILING_COUNTER_ADD(name, count)
#define PROFILING_COUNTER_INC(name)
#endif

// ----------------------- Project independent structures ----------------------

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

  TriangularMatrix(const TriangularMatrix &) = delete;
  TriangularMatrix &operator=(const TriangularMatrix &) = delete;
  TriangularMatrix(TriangularMatrix &&) = delete;
  TriangularMatrix &operator=(TriangularMatrix &&) = delete;

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
    assert(i < m_size &&j < m_size);
    if (i > j) std::swap(i, j);
    size_t idx = m_size * (m_size - 1) / 2 - (m_size - i) * (m_size - i - 1) / 2 + j - i - 1;
    return m_array[idx];
  }
};


/*
 * A bitset implementation, similar to std::bitset but with the 'contains(SpecificBitset)' operation,
 * replacing '(B & ~A).none()' which creates 3 more std::bitsets than necessary.
 */
template<size_t Size>
class SpecificBitSet {
private:
  using word_t = unsigned long long;
  static constexpr size_t WORD_SIZE = sizeof(word_t) * CHAR_BIT;
  static constexpr size_t WORD_COUNT = Size / WORD_SIZE;
  word_t m_array[WORD_COUNT]{};

public:
  inline bool isSet(size_t stationIdx) const
  {
    return m_array[stationIdx / WORD_SIZE] & (1ll << (stationIdx & (WORD_SIZE - 1)));
  }

  inline void setSet(size_t stationIdx)
  {
    m_array[stationIdx / WORD_SIZE] |= 1ll << (stationIdx & (WORD_SIZE - 1));
  }

  inline bool contains(const SpecificBitSet<Size> &other) const
  {
    for (size_t i = 0; i < WORD_COUNT; i++)
      if (other.m_array[i] & ~m_array[i])
        return false;
    return true;
  }

  inline word_t operator[](size_t idx) const { return m_array[idx]; }
};

/*
 * std::vector backed priority queue, with a maximal capacity.
 */
template<class T, typename Compare = std::less<T>>
class SmallBoundedPriorityQueue {
private:
  std::vector<T> m_queue;
public:
  SmallBoundedPriorityQueue(size_t capacity)
  {
    m_queue.reserve(capacity);
  }

  const T &top() const
  {
    return m_queue[0];
  }

  void pop()
  {
    assert(m_queue.size() > 0);
    m_queue.erase(m_queue.begin());
  }

  bool empty() const
  {
    return m_queue.empty();
  }

  size_t remaining() const
  {
    return m_queue.capacity() - m_queue.size();
  }

  template<class _Pr>
  void removeIf(_Pr predicate)
  {
    m_queue.erase(std::remove_if(m_queue.begin(), m_queue.end(), predicate), m_queue.end());
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
      if (Compare{}(x, m_queue[i])) { // TODO use a bisection here, could be O(logN) instead of O(N)
        if (m_queue.size() >= m_queue.capacity())
          m_queue.pop_back();
        // ...but this will always be O(N) with a std::vector, consider switching to a list or a template argument
        m_queue.insert(m_queue.begin() + i, std::move(x));
        return;
      }
    }
    if (m_queue.size() != m_queue.capacity())
      m_queue.push_back(std::move(x));
  }
};

/* comparator implementation that always considers that A<B, no matter A and B */
template<class S>
struct leftComp {
  constexpr bool operator()(const S &, const S &)
  {
    return true;
  }
};

template<class S>
using SmallBoundedQueue = SmallBoundedPriorityQueue<S, leftComp<S>>;

/*
 * An inline allocator is a structure that is able to differenciate
 * allocated and non-allocated memory for a specific structure.
 * It does not do proper allocation per-say but it helps allocators
 * that have no information of (non)allocated memory, like the Clock
 * Arena Allocator.
 */
template<typename T, typename S>
concept InlineAllocator = requires(const T & constAllocator, T & allocator, S & slot)
{
  { constAllocator.isFree(slot) } -> std::same_as<bool>;
  { allocator.setFree(&slot) };
  { allocator.setFree(&slot, &slot + 1) };
};

/* default inline allocator implementation, a slot is empty iff its first byte is 0xff */
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

  inline void setFree(S *fromSlot, S *toSlot)
  {
    memset(fromSlot, 0xff, (char*)toSlot-(char*)fromSlot);
  }
};

/*
 * TODO comment ClockArenaAllocator
 * 
 * Watch out! check that DefaultInlineAllocator<S> suits your needs before creating
 * ClockArenaAllocators without specifying an InlineAllocator.
 */
template<class S, typename index_t = size_t, InlineAllocator<S> InlineAllocator = DefaultInlineAllocator<S>>
class ClockArenaAllocator {
protected:
  static constexpr float FREE_SLOTS_THRESHOLD = .05f; // if there are less than 5% free slots left realloc into a bigger array
  
  InlineAllocator m_allocator;
  S              *m_array;
  index_t         m_next;
  size_t          m_size;
  size_t          m_freeSlotCount;

public:
  explicit ClockArenaAllocator(size_t size)
    : m_size(size), m_next(0), m_freeSlotCount(size)
  {
    m_array = (S *)malloc(sizeof(S) * size);
    if (!m_array) throw std::runtime_error("Out of memory on first allocation");
    m_allocator.setFree(&m_array[0], &m_array[size]);
  }

  ~ClockArenaAllocator()
  {
    std::free(m_array);
  }

  ClockArenaAllocator(const ClockArenaAllocator &) = delete;
  ClockArenaAllocator &operator=(const ClockArenaAllocator &) = delete;

  S &operator[](index_t idx) { assert(idx < m_size); return m_array[idx]; }
  const S &operator[](index_t idx) const { assert(idx < m_size); return m_array[idx]; }

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
      S *newArray = (S *)realloc(m_array, newSize * sizeof(S));
      if (!newArray)
        throw std::runtime_error("Out of memory");
      m_allocator.setFree(&newArray[m_size], &newArray[newSize]);
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

// ----------------------- Project independent functions ----------------------

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
 * Returns the position of the first bit set to 1 in x, or -1 if it is 0.
 * ie. first1bitIndex(0b1100) = 2
 */
template<typename T>
constexpr unsigned char first1bitIndex(T x)
{
  if (x == 0)
    return -1;
  for (unsigned char i = 0; i < sizeof(T) * CHAR_BIT; i++) {
    if (x & (1ull << i))
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

/*
 * A first bit lookup table of size S is a table that can be indexed by x<S and the retrieved value
 * is the position of the first bit set to 1 in the binary representation of x. Take care to index
 * with unsigned integers.
 */
template<size_t S>
constexpr std::array<unsigned char, S> createFirstBitLookupTable()
{
  std::array<unsigned char, S> lookupTable;
  for (size_t i = 0; i < S; i++)
    lookupTable[i] = first1bitIndex(i);
  return lookupTable;
}

long long currentTimeMs()
{
  namespace chr = std::chrono;
  long long ms = chr::duration_cast<chr::milliseconds>(chr::system_clock::now().time_since_epoch()).count();
  return ms;
}

}