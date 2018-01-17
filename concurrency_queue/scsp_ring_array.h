# ifndef _SCSP_RING_ARRAY_H_
# define _SCSP_RING_ARRAY_H_
//--------------------------------------------------------------------------------
# include <cstdlib>
# include <cassert>
# include <atomic>
# include <memory>
//--------------------------------------------------------------------------------
# include "utils/mem_utils.h"
//--------------------------------------------------------------------------------
namespace concur {
//--------------------------------------------------------------------------------

template < typename Type, std::size_t Alignment = 64 >
class ScspRingArray
{
private:
  class ALIGNAS( Alignment ) Node
  {
  public:
    Node( const Node& )             = delete;
    Node& operator =( const Node& ) = delete;
    
    Node( ) : flag( false ), data( ) { }
    
    template < typename T >
    bool put( T&& src )
    {
      if( flag.load( std::memory_order_acquire ) )
        return false;
      
      data = std::forward< T >( src );
      flag.store( true, std::memory_order_release );
      return true;
    }
    
    template < typename T >
    bool take( T& dst )
    {
      if( !flag.load( std::memory_order_acquire ) )
        return false;
      
      dst = std::move( data );
      flag.store( false, std::memory_order_release );
      return true;
    }
    
  private:
    std::atomic< bool > flag;
    Type                data;
  }; 
  
public:
  typedef uint_fast32_t     size_type;
  typedef Type              element_type;
  
  ScspRingArray( const ScspRingArray& )             = delete;
  ScspRingArray& operator =( const ScspRingArray& ) = delete;
  
public:
  ScspRingArray( ) = default;
  
  void init( unsigned size )
  {
    ring_.init( size );
  }

  template < typename T >
  bool push( T&& src )
  {
    if( at( head_ ).put( std::forward< T >( src ) ) ) {
      ++head_;
      return true;
    }
    return false;
  }

  template < typename T >
  bool pop( T& dst )
  {
    if( at( tail_ ).take( dst ) ) {
      ++tail_;
      return true;
    }
    return false;
  }

private:
  typedef utils::aligned_ring< Node, size_type > ring_type;
  
  inline Node& at( size_type i ) { return ring_.at( i ); }
  
  ring_type   ring_;
  size_type   head_   = 0;
  size_type   tail_   = 0;
}; // class ScspRingArray

//--------------------------------------------------------------------------------
} // namespace concur
//--------------------------------------------------------------------------------
# endif // _SCSP_RING_ARRAY_H_
