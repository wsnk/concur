# ifndef _SCMR_POOL_H_
# define _SCMR_POOL_H_
//--------------------------------------------------------------------------------
# include <cassert>
# include <atomic>
# include <utility>
//--------------------------------------------------------------------------------
# include "pool_node.h"
# include "pool_holder.h"
//--------------------------------------------------------------------------------
namespace concpool {
//--------------------------------------------------------------------------------

template < typename T >
class ScmrPool // Single consumer - multiple releasers
{
public:
  typedef T                               element_type;
  typedef details::Node< element_type >   node_type;
  typedef Holder< ScmrPool >              holder_type;
  
  friend holder_type;
  
  ScmrPool( const ScmrPool& )             = delete;
  ScmrPool& operator =( const ScmrPool& ) = delete;
  
public:
  ScmrPool( )
    : tail_( new node_type ), head_( tail_ )
  { }

  ~ScmrPool( )
  {
    while( node_type* n = pop_node( ) )
      delete n;
    delete tail_;
  }
  
  template < typename ...Args >
  void init( std::size_t count, Args ...args )
  {
    while( count-- )
      push_node( new node_type( args... ) );
  }
  
  template < typename ...Args >
  inline holder_type create( Args&& ...args )
  {
    return holder_type( this, new node_type( std::forward< Args >( args )... ) );
  }
  
  inline holder_type pop( )
  {
    return holder_type( this, pop_node( ) );
  }

private:
  void push_node( node_type* node ) 
  {
    assert( node->next.load( ) == nullptr );
    node_type* old_head = head_.exchange( node, std::memory_order_acq_rel );
    old_head->next.store( node, std::memory_order_release );
  }

  node_type* pop_node( ) 
  {
    node_type* node = tail_->next.exchange( nullptr, std::memory_order_acquire );
    if( node ) {
      std::swap( tail_, node );
      node->data = std::move( tail_->data );
    }
    return node;
  }

private:
  node_type*                 tail_; // pop elements from (used only by C-thread)
  std::atomic< node_type* >  head_; // release elements to (used only by R-threads)
}; // class ScmrPool

//--------------------------------------------------------------------------------
} // namespace concpool
//--------------------------------------------------------------------------------
# endif // _SCMP_POOL_H_

