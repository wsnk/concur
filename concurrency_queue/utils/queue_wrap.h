// concurrency/utils
//--------------------------------------------------------------------------------
# ifndef _QUEUE_WRAP_H_
# define _QUEUE_WRAP_H_
//--------------------------------------------------------------------------------
# include <queue>
//--------------------------------------------------------------------------------
namespace concur { namespace utils {
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
namespace details {
//--------------------------------------------------------------------------------

// Inserts item into a given queue if the queue size doesn't exceed limit.
template < bool Limited = true >
struct queue_pusher
{
  std::size_t limit;
  
  queue_pusher( ) : limit( 0 ) { }
  
  inline void set_limit( std::size_t v ) { limit = v; }
  
  template < typename QueueT, typename T >
  inline bool push( QueueT& queue, T&& val ) const {
    if( queue.size( ) < limit ) {
      queue.emplace( std::forward< T >( val ) );
      return true;
    }
    return false;
  }
};

// Inserts item into a given queue always.
template < >
struct queue_pusher< false >
{
  inline void set_limit( std::size_t ) { }
  
  template < typename QueueT, typename T >
  inline bool push( QueueT& queue, T&& val ) const {
    queue.emplace( std::forward< T >( val ) );
    return true;
  }
};

//--------------------------------------------------------------------------------
} // namespace details
//--------------------------------------------------------------------------------

template < typename Type, bool Limited = false >
class Queue : private details::queue_pusher< Limited >
{
public:
  typedef Type                              element_type;
  typedef std::queue< element_type >        queue_type;
  typedef typename queue_type::size_type    size_type;
  
  Queue( ) : max_( 0 ) { }
  
  inline void set_limit( size_type limit ) {
    pusher_type::set_limit( limit );
  }
  
  template < typename T >
  inline bool push( T&& src ) {
    return pusher_type::push( queue_, std::forward< T >( src ) );
  }
  
  template < typename T >
  inline bool pop( T& dst ) {
    if( !queue_.empty( ) ) {
      dst = std::move( queue_.front( ) );
      queue_.pop( );
      return true;
    }
    return false;
  }
  
  inline void clear( ) {
    queue_type tmp;
    queue_.swap( tmp );
  }
  
  inline size_type size( ) {
    return queue_.size( );
  }
  
private:
  typedef details::queue_pusher< Limited > pusher_type;
  
private:
  queue_type  queue_;
  size_type   max_;
}; // class Queue

//--------------------------------------------------------------------------------
} } // namespace concur.utils
//--------------------------------------------------------------------------------
# endif // _QUEUE_WRAP_H_
