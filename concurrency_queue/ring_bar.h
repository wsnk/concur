# ifndef _RING_BAR_H_
# define _RING_BAR_H_
//--------------------------------------------------------------------------------
# include <atomic>
# include "utils/mem_utils.h"
//--------------------------------------------------------------------------------
//
// The container is a sort of SCMP lock-free ring. Each thread obtains an element
// from the ring, do whatever it wants and release it afterward.
//
// Visitors fetch elements in arbitrary order. After releasing the elements became
// READY. Master always gets elements in order they were fetched by producers. In
// other words it can't get a READY element if there is a NOT-READY one before it.
// It means if someone gets stuck in processing or loses an obtained element, it
// may block the whole ring. Hence every thread must always release the elements
// and do it as fast as possible.
// 
// Type must have default constructor.
//
//--------------------------------------------------------------------------------
namespace concur {
//--------------------------------------------------------------------------------

template < typename Type, std::size_t Alignment >
class ALIGNAS( Alignment ) RingBar
{
private:
  struct ALIGNAS( Alignment ) Node
  {
    enum : uint_fast8_t { FREE, READY };
    
    Node( const Node& )             = delete;
    Node& operator =( const Node& ) = delete;
    
    Node( ) : state( FREE ), data( ) { }
    
    std::atomic< uint_fast8_t >   state;
    Type                          data;
  };
  
public:
  typedef RingBar< Type, Alignment >    this_type;
  typedef Type                          element_type;
  
  RingBar( const RingBar& )             = delete;
  RingBar& operator =( const RingBar& ) = delete;
  
public:
  RingBar( ) : free_count_( 0 ), head_( 0 ), tail_( 0 ) { }
  
  //----------------------------------------
  // not thread-safe
  
  void init( unsigned size )
  {
    ring_.init( size );
    free_count_.store( size );
  }
  
  inline element_type& at( std::size_t pos )
  {
    return ring_.at( static_cast< size_type >( pos ) ).data;
  }
  
  //----------------------------------------
  // for visitors
  
  element_type* visitor_fetch( )
  {
    if( free_count_.fetch_sub( 1, std::memory_order_acquire ) <= 0 ) {
      free_count_.fetch_add( 1, std::memory_order_relaxed );
      return nullptr;
    }
    const size_type i = head_.fetch_add( 1, std::memory_order_relaxed );
    return &( ring_.at( i ).data );
  }
  
  void visitor_release( const element_type* ptr )
  {
    to_node( ptr )->state.store( Node::READY, std::memory_order_release );
  }
  
  //----------------------------------------
  // for master
  
  element_type* master_fetch( )
  {
    Node& node = ring_.at( tail_ );
    if( node.state.exchange( Node::FREE, std::memory_order_acquire ) != Node::READY )
      return nullptr;
    ++tail_;
    return &( node.data );
  }
  
  void master_release( const element_type* )
  {
    free_count_.fetch_add( 1, std::memory_order_release );
  }
  
  void master_skip_all( )
  {
    while( ring_.at( tail_ ).state.exchange( Node::FREE, std::memory_order_relaxed ) == Node::READY ) {
      free_count_.fetch_add( 1, std::memory_order_relaxed );
      ++tail_;
    }
  }

private:
  static inline Node* to_node( const element_type* ptr )
  {
    return const_cast< Node* >(
          reinterpret_cast< const Node* >(
            reinterpret_cast< const char* >( ptr ) - offsetof( Node, data )
            )
          );
  }
  
  typedef uint_fast32_t                           size_type;
  typedef std::atomic< size_type >                atomic_size_type;
  typedef std::atomic< int_fast32_t >             atomic_counter_type;
  typedef utils::aligned_ring< Node, size_type >  ring_type;

private:
  ring_type            ring_;
  atomic_size_type     head_;
  ALIGNAS( Alignment )
  atomic_counter_type  free_count_;
  size_type            tail_;
}; // class RingBar

//--------------------------------------------------------------------------------
} // namespace concur
//--------------------------------------------------------------------------------
# endif // _RING_BAR_H_
