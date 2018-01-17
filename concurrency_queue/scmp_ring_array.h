# ifndef _SCMP_RING_ARRAY_H_
# define _SCMP_RING_ARRAY_H_
//--------------------------------------------------------------------------------
# include <cstdlib>
# include <cassert>
# include <atomic>
# include <memory>
//--------------------------------------------------------------------------------

# define CONCUR_ALIGNMENT 64

//--------------------------------------------------------------------------------

# ifdef _MSC_VER 
#   define ALIGNAS( V )   __declspec( align( 64 ) ) // unable to use constant in MSVC
#   define ALIGNOF( T )   __alignof( T )
# else
#   define ALIGNAS( V )   alignas( V )
#   define ALIGNOF( T )   alignof( T )
# endif

//--------------------------------------------------------------------------------
namespace concur {
//--------------------------------------------------------------------------------

template < typename T, std::size_t Alignment >
class ScmpRingArray
{
private:
  static const std::memory_order AQR = std::memory_order_acquire;
  static const std::memory_order RLS = std::memory_order_release;
  static const std::memory_order RLX = std::memory_order_relaxed;
  
  typedef uint_fast32_t               size_type;
  typedef std::atomic< size_type >    atomic_size_type;
  typedef std::atomic< int_fast32_t > atomic_counter_type;
  
  struct ALIGNAS( Alignment ) Node
  {
    Node( const Node& )             = delete;
    Node& operator =( const Node& ) = delete;
    
    Node( ) : flag( false ), data( ) { }
    
    std::atomic< bool > flag;
    T                   data;
  }; 
  
public:
  ScmpRingArray( const ScmpRingArray& )             = delete;
  ScmpRingArray& operator =( const ScmpRingArray& ) = delete;
  
  enum : unsigned {
    NODE_SIZE       = sizeof( Node ),
    NODE_ALIGNMENT  = ALIGNOF( Node )
  };
  
public:
  ScmpRingArray( )
    : mem_( nullptr ), arr_( nullptr ), arr_size_( 0 ), head_( 0 ), tail_( 0 )
  { }
  ~ScmpRingArray( ) { std::free( mem_ ); }
  
  void init( unsigned size )
  {
    assert( arr_ == nullptr );
    if( allocate_storage( size ) ) {
      pcounter_.store( arr_size_ = size );

      for( unsigned i( 0 ); i < size; ++i ) new( arr_ + i ) Node;
    }
  }

  template < typename Type >
  bool push( Type&& src )
  {
    if( pcounter_.fetch_sub( 1, AQR ) <= 0 ) {
      pcounter_.fetch_add( 1, RLX );
      return false;
    }
    
    Node& node = at( head_.fetch_add( 1, RLX ) );
    node.data = std::forward< Type >( src );
    node.flag.store( true, RLS );
    
    return true;
  }

  template < typename Type >
  bool pop( Type& dst )
  {
    bool expected = true;
    
    Node& node = at( tail_ );
    if( node.flag.compare_exchange_strong( expected, false, AQR, RLX ) ) {
      dst = std::move( node.data );
      pcounter_.fetch_add( 1, RLS );
      ++tail_;
    }
    
    return expected;
  }

private:
  inline Node& at( size_type i ) {
    return arr_[ i % arr_size_ ];
  }
  
  bool allocate_storage( unsigned size )
  {
    std::size_t mem_size = sizeof( Node ) * size + ALIGNOF( Node );
    if( ( mem_ = std::malloc( mem_size ) ) ) {
      const uint64_t addr     = reinterpret_cast< uint64_t >( mem_ );
      const uint64_t aligned  = addr + ( ALIGNOF( Node ) - addr % ALIGNOF( Node ) );
      assert( ( aligned >= addr ) && !( aligned % ALIGNOF( Node ) ) );
      arr_ = reinterpret_cast< Node* >( aligned );
      return true;
    }
    return false;
  }
  
  void*                           mem_;
  Node*                           arr_;
  size_type                       arr_size_;
  ALIGNAS( Alignment )   atomic_counter_type   pcounter_;
  ALIGNAS( Alignment )   atomic_size_type      head_;
  ALIGNAS( Alignment )   size_type             tail_;
}; // class ScmpRingArray

//--------------------------------------------------------------------------------
} // namespace concur
//--------------------------------------------------------------------------------
# endif // _SCMP_RING_ARRAY_H_
