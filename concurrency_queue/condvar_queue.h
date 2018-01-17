# ifndef _CONDVAR_QUEUE_H_
# define _CONDVAR_QUEUE_H_
//--------------------------------------------------------------------------------
# include <cassert>
//--------------------------------------------------------------------------------
# include <mutex>
# include <condition_variable>
# include <queue>
//--------------------------------------------------------------------------------
# include "non_pod_utils.h"
# include "utils/queue_wrap.h"
//--------------------------------------------------------------------------------
namespace concur {
//--------------------------------------------------------------------------------
// simple concurrent containers with mutex and conditional variable

template < typename Type, bool Limited = false >
class CondvarQueue
{ 
public:
  typedef Type element_type;
  
  CondvarQueue( const CondvarQueue& )               = delete;
  CondvarQueue& operator =( const CondvarQueue& )   = delete;
  
  CondvarQueue( ) = default;
  
  inline void init( std::size_t capacity )
  {
    queue_.set_limit( capacity );
  }
  
  template < typename T >
  bool push( T&& val )
  {
    lock_type lock( mtx_ );
    if( queue_.push( std::forward< T >( val ) ) ) {
      condvar_.notify_one( );
      return true;
    }
    return false;
  }
  
  template < typename T >
  bool pop( T& dst )
  {
    lock_type lock( mtx_ );
    return queue_.pop( dst );
  }
  
  template < typename T, class Rep, class Period >
  bool pop( T& dst, const std::chrono::duration< Rep, Period > dur )
  {
    lock_type lock( mtx_ );
    while( !queue_.pop( dst ) ) {
      if( condvar_.wait_for( lock, dur ) == std::cv_status::timeout )
        return false;
    }
    return true;
  }
  
  template < typename T >
  inline bool pop( T& dst, unsigned microsec_dur )
  {
    return pop( dst, std::chrono::microseconds( microsec_dur ) );
  }
  
  inline void clear( )
  {
    lock_type lock( mtx_ );
    queue_.clear( );
  }
  
  inline std::size_t size( ) const
  {
    lock_type lock( mtx_ );
    return queue_.size( );
  }

private:
  typedef std::unique_lock< std::mutex >          lock_type;
  typedef utils::Queue< element_type, Limited >   queue_type;
  
private:
  mutable std::mutex        mtx_;
  std::condition_variable   condvar_;
  queue_type                queue_;
}; // class CondvarQueue

//--------------------------------------------------------------------------------

template < typename T >
class CondvarRingArray
{ 
public:
  typedef T             element_type;
  typedef std::size_t   size_type;
  
  CondvarRingArray( const CondvarRingArray& )               = delete;
  CondvarRingArray& operator =( const CondvarRingArray& )   = delete;
  
  CondvarRingArray( ) : arr_( nullptr ) { }
  ~CondvarRingArray( ) { delete[ ] arr_; }
  
  void init( size_type size )
  {
    assert( !arr_ && "array must be empty" );
    arr_ = new element_type[ arr_size_ = size ];
    count_ = tail_ = head_ = 0;
  }
  
  template < typename Type >
  bool push( Type&& src )
  {
    bool result = false;
    {
      lock_type lock( mtx_ );
      result = put_element( std::forward< Type >( src ) );
    }
    if( result ) condvar_.notify_one( );
    return result;
  }
  
  template < typename Type >
  bool pop( Type& dst )
  {
    lock_type lock( mtx_ );
    return get_element( dst );
  }
  
  template < typename Type, class Rep, class Period >
  bool pop( Type& dst, const std::chrono::duration< Rep, Period > dur )
  {
    lock_type lock( mtx_ );
    while( !get_element( dst ) ) {
      if( condvar_.wait_for( lock, dur ) == std::cv_status::timeout )
        return false;
    }
    return true;
  }
  
  template < typename Type >
  inline bool pop( Type& dst, unsigned microsec_dur )
  {
    return pop( dst, std::chrono::microseconds( microsec_dur ) );
  }
  
  void clear( )
  {
    lock_type lock( mtx_ );
    DestructorCaller< std::is_pod< element_type >::value >::destroy( arr_, arr_size_ );
    count_ = tail_ = head_ = 0;
  }
  
  size_type size( ) const
  {
    lock_type lock( mtx_ );
    return count_;
  }

private:
  typedef std::unique_lock< std::mutex >  lock_type;
  
  inline element_type& at( size_type i )
  {
    return arr_[ i % arr_size_ ];
  }
  
  template < typename Type >
  inline bool put_element( Type&& src )
  {
    if( count_ < arr_size_) {
      at( head_++ ) = std::forward< Type >( src );
      ++count_;
      return true;
    }
    return false;
  }
  
  template < typename Type >
  inline bool get_element( Type& dst )
  {
    if( count_ > 0 ) {
      dst = std::move( at( tail_++ ) );
      --count_;
      return true;
    }
    return false;
  }
  
private:
  element_type*             arr_;
  std::size_t               arr_size_;
  mutable std::mutex        mtx_;
  std::condition_variable   condvar_;
  std::size_t               count_;
  std::size_t               head_;
  std::size_t               tail_;
}; // class CondvarRingArray

//--------------------------------------------------------------------------------
} // namespace concur
//--------------------------------------------------------------------------------
# endif // _CONDVAR_QUEUE_H_
