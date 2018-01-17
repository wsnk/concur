#ifndef MCMP_LIST_H__
#define MCMP_LIST_H__
//------------------------------------------------------------------------------
#include <atomic>
#include <mutex>
#include <new>
//------------------------------------------------------------------------------
template < typename T >
struct MCMPListItem {
  typedef std::atomic< MCMPListItem< T > * > Next;

  Next next;
  T data;
};
//------------------------------------------------------------------------------
template < typename T, typename Allocator = std::allocator< MCMPListItem<T> > >
class MCMPList {
public:
  typedef T ElementType;
public:
  MCMPList() : fRead(&fRoot), fTail(&fRoot) {
    fRoot.next = &fRoot;
  }
  ~MCMPList() {
    Allocator allocator;
    auto ptr = fRoot.next.load(std::memory_order_relaxed);
    while (ptr != &fRoot) {
      auto const next = ptr->next.load(std::memory_order_relaxed);      
      ptr->~ListItem();
      allocator.deallocate(ptr, 1);
      ptr = next;
    }
  }

  bool consume(T &aOut) {
    std::lock_guard< std::mutex > lock(fConsumeMutex);

    bool success = false;

    auto const read = fRead.load(std::memory_order_relaxed);
    auto const next = read->next.load(std::memory_order_relaxed);
    if (&fRoot != next) {
      aOut = std::move(next->data);
      fRead.store(next, std::memory_order_relaxed);

      success = true;
    }

    return success;
  }

  void produce(T const &aIn) {
    emplace(aIn);
  }

  template < typename... Args >
  void emplace(Args&&... aArgs) {
    Allocator allocator;

    auto newItem = allocator.allocate(1);
    new ( &(newItem->next) ) typename ListItem::Next();
    newItem->next.store(&fRoot, std::memory_order_relaxed);
    new ( &(newItem->data) ) T(std::forward< Args >(aArgs)...);

    std::unique_lock< std::mutex > lock(fProduceMutex);
    // push
    fTail->next.store(newItem, std::memory_order_relaxed);
    fTail = newItem;

    // garbage collect
    auto read = fRead.load(std::memory_order_relaxed);
    auto erase = read;
    if (read != &fRoot) {
      erase = fRoot.next.load(std::memory_order_relaxed);
      if (erase != read) {
        fRoot.next.store(read, std::memory_order_relaxed);
      }
    }
    lock.unlock();

    // garbage collect
    while (erase != read) {
      auto const next = erase->next.load(std::memory_order_relaxed);
      erase->~ListItem();
      allocator.deallocate(erase, 1);
      erase = next;
    }
  }
private:
  typedef MCMPListItem< T > ListItem;
private:
  std::mutex fConsumeMutex;
  std::mutex fProduceMutex;
  ListItem fRoot;
  std::atomic< ListItem * > fRead;
  ListItem *fTail;
};
//------------------------------------------------------------------------------
#endif
