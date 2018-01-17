# ifndef _MT_PTR_RING_POOL_H_
# define _MT_PTR_RING_POOL_H_
//--------------------------------------------------------------------------------
# include <cassert>
# include <memory>
# include <mutex>
# include <atomic>
//--------------------------------------------------------------------------------
namespace concpool {
//--------------------------------------------------------------------------------

template < typename Type >
class PtrRingPool
{
public:
  typedef Type                                                  element_type;
  typedef typename std::remove_extent< element_type >::type*    pointer_type;
  
  PtrRingPool( const PtrRingPool& )               = delete;
  PtrRingPool& operator =( const PtrRingPool& )   = delete;
  PtrRingPool( )                                  = default;
  
public:
  ~PtrRingPool( )
  {
    std::default_delete< element_type > deleter;
    for( unsigned i( 0 ); i < size_; ++i )
      deleter( ring_[ i ] );
  }
  
  void alloc( unsigned size )
  {
    assert( ring_ == nullptr );
    
    ring_ = new pointer_type[ size ];
    for( unsigned i( 0 ); i < size; ++i )
      ring_[ i ] = nullptr;
    size_ = size;
    tail_ = 0;
    head_ = 0;
  }
  
  inline pointer_type take( )
  {
    pointer_type val = nullptr;
    {
      lock_type lk( mtx_ );
      std::swap( val, at( tail_ ) );
      if( val ) ++tail_;
    }
    return val;
  }
  
  inline void release( pointer_type ptr )
  {
    lock_type lk( mtx_ );
    at( head_++ ) = ptr;
  }
  
private:
  typedef std::atomic< pointer_type >     atomic_pointer_type;
  typedef std::mutex                      mutex_type;
  typedef std::lock_guard< mutex_type >   lock_type;
  
  inline pointer_type& at( unsigned i )
  {
    return ring_[ i % size_ ];
  }
  
private:
  std::mutex            mtx_;
  atomic_pointer_type*  ring_ = nullptr;
  unsigned              size_ = 0;
  unsigned              head_ = 0;
  unsigned              tail_ = 0;
}; // class PtrRingPool

//--------------------------------------------------------------------------------
} // namespace concur
//--------------------------------------------------------------------------------

# endif // _MT_PTR_RING_POOL_H_
