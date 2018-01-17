# ifndef _POOL_NODE_H_
# define _POOL_NODE_H_
//--------------------------------------------------------------------------------
# include <atomic>
//--------------------------------------------------------------------------------
namespace concpool { namespace details {
//--------------------------------------------------------------------------------

template < typename T >
struct Node
{
  typedef T element_type;
  
  std::atomic< Node* >  next;
  element_type          data;

  Node( const Node& )             = delete;
  Node& operator =( const Node& ) = delete;

  template < typename ...Args >
  Node( Args&& ...args )
    : next( nullptr ), data( std::forward< Args >( args )... )
  { }
};

//--------------------------------------------------------------------------------
} } // namespace concpool::details
//--------------------------------------------------------------------------------
# endif // _POOL_NODE_H_

