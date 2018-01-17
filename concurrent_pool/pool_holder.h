# ifndef _POOL_HOLDER_H_
# define _POOL_HOLDER_H_
//--------------------------------------------------------------------------------
# include <cassert>
# include <utility>
//--------------------------------------------------------------------------------
namespace concpool {
//--------------------------------------------------------------------------------

template < typename PoolType >
class Holder
{
public:
  typedef PoolType                          pool_type;
  typedef typename pool_type::element_type  element_type;
  typedef typename pool_type::node_type     node_type;
  
public:
  Holder( ) : pool_( nullptr ), node_( nullptr ) { }
  Holder( pool_type* pool, node_type* node ) : pool_( pool ), node_( node ) { }
  
  ~Holder( )
  {
    if( node_ && pool_ )
      pool_->push_node( node_ );
    else
      delete node_;
  }
  
  Holder( Holder&& x ) : Holder( )
  {
    operator =( std::move( x ) );
  }
  
  Holder& operator =( Holder&& x )
  {
    std::swap( pool_, x.pool_ );
    std::swap( node_, x.node_ );
    return *this;
  }
  
  operator bool ( ) const { return node_; }

  inline element_type*   operator ->( )  const { return get( ); }
  inline element_type&   operator *( )   const { return *get( ); }
  
  inline element_type* get( ) const
  {
    assert( *this );
    return &( node_->data );
  }
  
private:
  pool_type*    pool_;
  node_type*    node_;
}; // class Holder

//--------------------------------------------------------------------------------
} // namespace concpool
//--------------------------------------------------------------------------------
# endif // _POOL_HOLDER_H_

