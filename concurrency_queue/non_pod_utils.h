#ifndef NON_POD_UTILS_H__
#define NON_POD_UTILS_H__
//------------------------------------------------------------------------------
#include <new>
#include <type_traits>
#include <mutex>
//------------------------------------------------------------------------------
template < bool IsPOD >
struct ConstructorCaller {
  template < typename T >
  inline static void construct(T * const aBegin, size_t const aCount) {
    for (size_t i = 0; i < aCount; ++i) {
      new(aBegin + i) T();
    }
  }
};
//------------------------------------------------------------------------------
template < >
template < typename T >
inline void ConstructorCaller< true >::construct(T * const , size_t const )
{
  // nothing
}
//------------------------------------------------------------------------------
template < bool IsPOD >
struct DestructorCaller {
  template < typename T >
  inline static void destroy(T * const aBegin, size_t const aCount) {
    for (size_t i = 0; i < aCount; ++i) {
      (aBegin + i)->~T();
    }
  }
  template < typename T, typename MutexT >
  inline static void lock_and_destroy(MutexT & aMutex, T * const aBegin, size_t const aCount) {
    std::lock_guard< MutexT > lock( aMutex );
    for (size_t i = 0; i < aCount; ++i) {
      (aBegin + i)->~T();
    }
  }
};
//------------------------------------------------------------------------------
template < >
template < typename T >
inline void DestructorCaller< true >::destroy(T * const , size_t const )
{
  // nothing
}
//------------------------------------------------------------------------------
template < >
template < typename T, typename MutexT >
inline void DestructorCaller< true >::lock_and_destroy(MutexT & aMutex, T * const , size_t const )
{
  // nothing
}
//------------------------------------------------------------------------------
#endif
