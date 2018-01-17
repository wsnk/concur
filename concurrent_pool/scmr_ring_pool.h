# ifndef _SCMR_RING_POOL_H_
# define _SCMR_RING_POOL_H_
//--------------------------------------------------------------------------------
# include <atomic>
# include <stdexcept>
//--------------------------------------------------------------------------------
# include <utils/mem_utils.h> // from concurrent_queue
//--------------------------------------------------------------------------------
namespace concpool {
//--------------------------------------------------------------------------------

template < typename Type, std::size_t Alignment = 64 >
class ScmrRingPool
{
private:
  struct ALIGNAS( Alignment ) Node
  {
    Node( const Node& )             = delete;
    Node& operator =( const Node& ) = delete;
    
    Node( ) : ptr( nullptr ) { }
    
    std::atomic< Type* > ptr;
  }; 
  
  typedef uint_fast32_t                                   size_type;
  typedef concur::utils::aligned_ring< Node, size_type >  ring_type;
  typedef std::atomic< size_type >                        atomic_size_type;

public:
  typedef Type element_type;
  
  ScmrRingPool( const ScmrRingPool& )             = delete;
  ScmrRingPool& operator =( const ScmrRingPool& ) = delete;
  
public:
  ScmrRingPool( ) : cnum_( 0 ), rnum_( 0 ) { }
  ~ScmrRingPool( )
  {
    for( size_type i( 0 ); i < ring_.size( ); ++i ) {
      Type* ptr = ring_.at( i ).ptr.load( );
      ptr->~Type( );
      std::free( ptr );
    }
  }
  
  template < typename ...Args >
  void init( unsigned count, std::size_t element_size, Args ...args )
  {
    ring_.init( count );
    for( size_type i( 0 ); i < count; ++i ) {
      void* ptr = static_cast< Type* >( std::malloc( element_size ) );
      if( !ptr )
        throw std::bad_alloc( );
      ring_.at( i ).ptr.store( new( ptr ) Type( args... ) );
    }
  }

  template < typename T >
  inline void release( T* ptr )
  {
    Node& node = ring_.at( rnum_.fetch_add( 1, std::memory_order_relaxed ) );
    node.ptr.store( ptr, std::memory_order_release );
  }

  inline Type* pop( )
  {
    Type* ptr = ring_.at( cnum_ ).ptr.exchange( nullptr, std::memory_order_acquire );
    if( ptr )
      ++cnum_;
    return ptr;
  }
  
private:
  ring_type             ring_;
  size_type             cnum_ = 0; // consume number
  atomic_size_type      rnum_ = 0; // release number
}; // class ScmpRingArray

//--------------------------------------------------------------------------------
} // namespace concpool
//--------------------------------------------------------------------------------
# endif // _SCMR_RING_POOL_H_

