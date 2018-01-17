#ifndef MCMP_QUEUE_SEQ_H__
#define MCMP_QUEUE_SEQ_H__
//------------------------------------------------------------------------------
#include <atomic>
#include <mutex>
#include "non_pod_utils.h"
//------------------------------------------------------------------------------
template < typename T, typename Allocator = std::allocator< T > >
class MCMPSeq {
public:
  typedef T ElementType;
public:
  explicit MCMPSeq(int64_t const aQueueSize) : fSeq(0), fQueueSize(aQueueSize),
    fReader(0), fWriter(0)
  {
    fSeq = Allocator().allocate(fQueueSize);
    ConstructorCaller< std::is_pod< T >::value >::construct(fSeq, aQueueSize);
  }
  ~MCMPSeq() {
    DestructorCaller< std::is_pod< T >::value >::destroy(fSeq, fQueueSize);
    Allocator().deallocate(fSeq, fQueueSize);
  }

  bool produce(T const &aIn) {
    return emplace(aIn);
  }

  template< typename... Args >
  bool emplace(Args&&... aArgs) {
    std::lock_guard< std::mutex > lock(fWriteMutex);

    bool success = false;
    
    int64_t writer = 0;
    auto const fillCount = fill(0, &writer);
    if (fillCount < fQueueSize) {
      auto const writerIndex = writer % fQueueSize;
      new (&fSeq[writerIndex]) T(std::forward< Args >(aArgs)...);

      if (std::numeric_limits< int64_t >::max() == writer) {
        fWriter.store(0, std::memory_order_relaxed);
      } else {
        fWriter.store(writer + 1, std::memory_order_relaxed);
      }

      success = true;
    }

    return success;  
  }
  
  bool consume(T &aOut) {
    std::lock_guard< std::mutex > lock(fReadMutex);

    bool success = false;
    
    int64_t reader = 0;
    auto const fillCount = fill(&reader, 0);
    if (fillCount > 0) {
      auto const readerIndex = reader % fQueueSize;
      aOut = std::move(fSeq[readerIndex]);

      if (std::numeric_limits< int64_t >::max() == reader) {
        fReader.store(0, std::memory_order_relaxed);
      } else {
        fReader.store(reader + 1, std::memory_order_relaxed);
      }

      success = true;
    }

    return success;
  }

  int64_t fillCount() const {
    return fill(0, 0);
  }

  bool isFull() const {
    return (fillCount() == fQueueSize);
  }

  bool isEmpty() const {
    return (fillCount() == 0);
  }
private:
  int64_t fill(int64_t *aReader, int64_t *aWriter) const {
    auto const reader = fReader.load(std::memory_order_relaxed);
    auto const writer = fWriter.load(std::memory_order_relaxed);
    if (aReader) {
      *aReader = reader;
    }
    if (aWriter) {
      *aWriter = writer;
    }

    return (writer - reader);
  }
private:
  T *fSeq;
  int64_t const fQueueSize;
  std::mutex fReadMutex;
  std::mutex fWriteMutex;
  std::atomic< uint64_t > fReader;
  std::atomic< uint64_t > fWriter;
};//------------------------------------------------------------------------------
#endif
