# ifndef _SCSP_PTR_RING_ARRAY_H_
# define _SCSP_PTR_RING_ARRAY_H_
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
class ScspPtrRingArray
{
public:
  typedef Type*               element_type;
  typedef uint_fast32_t       size_type;
  
  ScspPtrRingArray( const ScspPtrRingArray& )             = delete;
  ScspPtrRingArray& operator =( const ScspPtrRingArray& ) = delete;
  
private:
  struct ALIGNAS( Alignment ) Node
  {
    Node( const Node& )             = delete;
    Node& operator =( const Node& ) = delete;
    
    Node( ) : data( nullptr ) { }
    
    inline bool put( Type* ptr ) {
      Type* expected = nullptr;
      return data.compare_exchange_strong( expected, ptr, std::memory_order_release );
    }
    
    inline bool take( Type*& ptr ) {
      return ( ptr = data.exchange( nullptr, std::memory_order_acquire ) );
    }
    
    std::atomic< Type* > data;
  };
  
public:
  ScspPtrRingArray( ) = default;
  
  void init( unsigned size )
  {
    ring_.init( size );
  }

  bool push( Type* ptr )
  {
    if( at( head_ ).put( ptr ) ) {
      ++head_;
      return true;
    }
    return false;
  }

  bool pop( Type*& ptr )
  {
    if( at( tail_ ).take( ptr ) ) {
      ++tail_;
      return true;
    }
    return false;
  }

private:
  typedef utils::aligned_ring< Node, size_type >   ring_type;
  
  inline Node& at( size_type i ) { return ring_.at( i ); }
  
  ring_type   ring_;
  size_type   head_  = 0;  // for producer
  size_type   tail_  = 0;  // for consumer
}; // class ScspPtrRingArray

//--------------------------------------------------------------------------------
} // namespace concur
//--------------------------------------------------------------------------------
# endif // _SCSP_PTR_RING_ARRAY_H_
