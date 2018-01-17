# ifndef _MT_CONTAINERS_H_
# define _MT_CONTAINERS_H_
//--------------------------------------------------------------------------------
# include <cassert>
# include <queue>
# include <mutex>
//--------------------------------------------------------------------------------
# include "non_pod_utils.h"
# include "container_grab.h"
# include "utils/queue_wrap.h"
//--------------------------------------------------------------------------------
namespace concur {
//--------------------------------------------------------------------------------
// simple thread-safe containers with mutex

template < typename Type, typename MutexType = std::mutex, bool Limited = false >
class Queue
{ 
public:
  typedef Type                      element_type;
  typedef MutexType                 mutex_type;
  typedef ContainerGrab< Queue >    grab_type;
  
  Queue( const Queue& )               = delete;
  Queue& operator =( const Queue& )   = delete;
  
  Queue( ) = default;
  
  inline void init( std::size_t capacity )
  {
    queue_.set_limit( capacity );
  }
  
  grab_type grab( )
  {
    return grab_type( *this );
  }
  
  template < typename T >
  bool push( T&& src )
  {
    lock_type lk( mtx_ );
    return put_element( std::forward< T >( src ) );
  }
  
  template < typename T >
  bool pop( T& dst )
  {
    lock_type lk( mtx_ );
    return get_element( dst );
  }
  
  std::size_t size( ) const
  {
    lock_type lk( mtx_ );
    return get_size( );
  }
  
  void clear( )
  {
    lock_type lk( mtx_ );
    queue_.clear( );
  }

private:
  friend grab_type;
  
  void lock( )   const { mtx_.lock( ); }
  void unlock( ) const { mtx_.unlock( ); }

  inline std::size_t get_size( ) const
  {
    return queue_.size( );
  }
  
  template < typename T >
  inline bool get_element( T& dst )
  {
    return queue_.pop( dst );
  }
  
  template < typename T >
  inline bool put_element( T&& src )
  {
    return queue_.push( std::forward< T >( src ) );
  }
  
private:
  typedef std::unique_lock< mutex_type >          lock_type;
  typedef utils::Queue< element_type, Limited >   queue_type;
  
private:
  mutable mutex_type        mtx_;
  queue_type                queue_;
}; // class Queue

//--------------------------------------------------------------------------------

template < typename Type, typename MutexType = std::mutex >
class RingArray
{ 
public:
  typedef Type                          element_type;
  typedef MutexType                     mutex_type;
  typedef std::size_t                   size_type;
  typedef ContainerGrab< RingArray >    grab_type;
  
  RingArray( const RingArray& )               = delete;
  RingArray& operator =( const RingArray& )   = delete;
  
  RingArray( )
    : arr_( nullptr ), arr_size_( 0 ), count_( 0 ), head_( 0 ), tail_( 0 )
  { }
  ~RingArray( ) { delete[ ] arr_; }
  
  void init( size_type size )
  {
    assert( !arr_ && "array must be empty" );
    arr_ = new element_type[ arr_size_ = size ];
  }
           
  grab_type grab( )
  {
    return grab_type( *this );
  }
  
  template < typename T >
  bool push( T&& val )
  {
    lock_type lk( mtx_ );
    return put_element( std::forward< T >( val ) );
  }
  
  bool pop( Type& dst )
  {
    lock_type lk( mtx_ );
    return get_element( dst );
  }
  
  size_type size( ) const
  {
    lock_type lk( mtx_ );
    return get_size( );
  }
    
  void clear( )
  {
    DestructorCaller< std::is_pod< element_type >::value >
      ::lock_and_destroy( mtx_, arr_, arr_size_ );
  }
  
private:
  friend grab_type;
    
  void lock( )    const { mtx_.lock( ); }
  void unlock( )  const { mtx_.unlock( ); }
  
  std::size_t get_size( ) const { return count_; }
    
  template < typename T >
  bool get_element( T& dst )
  {
    if( count_ == 0 ) return false;
    dst = std::move( at( tail_++ ) );
    --count_;
    return true;
  }
  
  template < typename T >
  bool put_element( T&& src )
  {
    if( count_ == arr_size_ ) return false;
    at( head_++ ) = std::forward< T >( src );
    ++count_;
    return true;
  }
    
private:
  typedef std::unique_lock< mutex_type >  lock_type;
  typedef std::queue< element_type >      queue_type;
  
  inline element_type& at( size_type i ) {
    return arr_[ i % arr_size_ ];
  }
  
private:
  mutable mutex_type        mtx_;
  element_type*             arr_;
  size_type                 arr_size_;
  size_type                 count_;
  size_type                 head_;
  size_type                 tail_;
}; // class RingArray

//--------------------------------------------------------------------------------
} // namespace concur
//--------------------------------------------------------------------------------
# endif // _MT_CONTAINERS_H_
