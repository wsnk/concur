# ifndef _SCMR_OCTOPUS_POOL_H_
# define _SCMR_OCTOPUS_POOL_H_
//--------------------------------------------------------------------------------
# include <cassert>
# include <atomic>
# include <utility>
# include <memory>
//--------------------------------------------------------------------------------
# include "scsr_pool.h"
//--------------------------------------------------------------------------------
namespace concpool {
//--------------------------------------------------------------------------------

template < typename Type, unsigned Alignment = 64 >
class ScmrOctopusPool
{
public:
  typedef Type element_type;

  ScmrOctopusPool( const ScmrOctopusPool& )             = delete;
  ScmrOctopusPool& operator =( const ScmrOctopusPool& ) = delete;
  
public:
  ScmrOctopusPool( ) : pools_( nullptr ) { }
  
  template < typename ...Args >
  void init( unsigned rcount, std::size_t count, std::size_t element_size, Args ...args )
  {
    assert( !pools_ && "pool already initialized" );
    
    pools_.reset( new pool_type[ rcount ] );
    for( unsigned i( 0 ); i < rcount; ++i ) {
      pool_type&  pool     = pools_[ i ];
      std::size_t elements = count / ( rcount - i );
      pool.init( elements, element_size, args... );
      count -= elements;
    }
    
    rcount_ = rcount;
    cnum_   = 0;
  }
  
  inline element_type* pop( )
  {
    const unsigned end = cnum_ + rcount_;
    do {
      if( element_type* element = pools_[ cnum_++ % rcount_ ].pop( ) )
        return element;
    } while( cnum_ != end );
    return nullptr;
  }
  
  inline void release( unsigned rnum, element_type* ptr )
  {
    pools_[ rnum ].release( ptr );
  }

private:
  typedef ScsrPool< element_type, Alignment > pool_type;
  
  std::unique_ptr< pool_type[ ] >   pools_;
  unsigned                          rcount_ = 0;
  unsigned                          cnum_   = 0;
}; // class ScmrPool


//--------------------------------------------------------------------------------
} // namespace concpool
//--------------------------------------------------------------------------------
# endif // _SCMR_OCTOPUS_POOL_H_

