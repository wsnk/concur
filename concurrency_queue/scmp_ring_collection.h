# ifndef _SCMP_RING_COLLECTION_H_
# define _SCMP_RING_COLLECTION_H_
//--------------------------------------------------------------------------------
# include "ring_bar.h"
//--------------------------------------------------------------------------------
namespace concur {
//--------------------------------------------------------------------------------

template < typename Type, std::size_t Alignment >
class ScmpRingCollection : public RingBar< Type, Alignment >
{
public:
  typedef typename RingBar< Type, Alignment >::element_type element_type;
  
public:
  ScmpRingCollection( ) : RingBar< Type, Alignment >( ) { }
  
  inline element_type* producer_fetch( ) {
    return this->visitor_fetch( );
  }
  
  inline void producer_release( const element_type* ptr ) {
    this->visitor_release( ptr );
  }
  
  inline element_type* consumer_fetch( ) {
    return this->master_fetch( );
  }
  
  inline void consumer_release( const element_type* ptr ) {
    this->master_release( ptr );
  }
  
  inline void consumer_skip_all( ) {
    this->master_skip_all( );
  }
}; // class ScmpRingCollection

//--------------------------------------------------------------------------------
} // namespace concur
//--------------------------------------------------------------------------------
# endif // _SCMP_RING_COLLECTION_H_
