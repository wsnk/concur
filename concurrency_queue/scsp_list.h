#ifndef SCSP_LIST_H__
#define SCSP_LIST_H__
//------------------------------------------------------------------------------
#include <atomic>
#include <memory>
#include <new>
//------------------------------------------------------------------------------
template < typename T >
struct SCSPListItem {
  typedef std::atomic< SCSPListItem< T > * > Next;

  Next next;
  T data;
};
//------------------------------------------------------------------------------
template < typename T, typename Allocator = std::allocator< SCSPListItem<T> > >
class SCSPList {
public:
  typedef T ElementType;
  typedef SCSPListItem< T > ListItem;
public:
  SCSPList() : fRead(&fRoot), fTail(&fRoot) {
    fRoot.next = &fRoot;
  }
  ~SCSPList() {
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

    // push
    auto newItem = allocator.allocate(1);
    new( &(newItem->next) ) typename ListItem::Next();
    newItem->next.store(&fRoot, std::memory_order_relaxed);
    new( &(newItem->data) ) T(std::forward< Args >(aArgs)...);

    fTail->next.store(newItem, std::memory_order_relaxed);
    fTail = newItem;

    // garbage collect
    auto const read = fRead.load(std::memory_order_relaxed);
    if (read != &fRoot) {
      ListItem *erase = fRoot.next.load(std::memory_order_relaxed);
      fRoot.next.store(read, std::memory_order_relaxed);
      while (erase != read) {
        auto const next = erase->next.load(std::memory_order_relaxed);
        erase->~ListItem();
        allocator.deallocate(erase, 1);
        erase = next;
      }
    }
  }
private:
  ListItem fRoot;
  std::atomic< ListItem * > fRead;
  ListItem *fTail;
};
//------------------------------------------------------------------------------
#endif
