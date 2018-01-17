# ifndef _SCMP_QUEUE_H_
# define _SCMP_QUEUE_H_
//--------------------------------------------------------------------------------
# include <cassert>
# include <atomic>
//--------------------------------------------------------------------------------
namespace concur {
//--------------------------------------------------------------------------------

/* FastMP-SlowSC queue
 * 
 * This queue intended to provide fast insertion for P-threads. There are only two
 * atomic operations in "push". Extraction is also fast, but can be erroneously
 * rejected if C-thread reaches a gap. Furthermore, if P-thread which made the gap
 * is interrupted, C-thread will be rejected on "pop" until exactly this producer
 * has fixed it.
 * Anyway both "push" and "pop" operations are wait-free.
 * 
 * Some restriction:
 * - Data type must has default constructor
 * - Data stays alive in a node until next one is popped
 * - Data is pushed and popped with move-assign operator
*/

template < typename T >
class ScmpQueue
{
public:
  ScmpQueue( const ScmpQueue& )             = delete;
  ScmpQueue& operator =( const ScmpQueue& ) = delete;

  ScmpQueue( )
    : head_( new Node ), tail_( head_ )
  { }

  ~ScmpQueue( )
  {
    while( pop_head( ) ) ;
    delete head_;
  }

  template < typename ...Args >
  bool push( Args&& ...args )
  {
    Node* new_node = new Node( std::forward<Args>( args )... );
    push_node( new_node );
    return true;
  }

  bool pop( T& dst )
  {
    Node* last = pop_head( );
    if( last ) {
      dst = std::move( last->data );
      return true;
    }
    return false;
  }

private:
  struct Node
  {
    std::atomic<Node*>  next;
    T                   data;

    Node( const Node& )             = delete;
    Node& operator =( const Node& ) = delete;

    template < typename ...Args >
    Node( Args&& ...args )
      : next( nullptr ), data( std::forward<Args>( args )... )
    { }
  };

  inline void push_node( Node* new_tail ) 
  {
    Node* old_tail = tail_.exchange( new_tail, std::memory_order_acq_rel ); // <TEAR>
    // now the queue is torn apart: a gap between <old_tail> and <new_tail>
    
    assert( old_tail                    && "<old_tail> must always exist" );
    assert( !( old_tail->next.load( ) ) && "<old_tail> must not have <next>" );
    
    old_tail->next.store( new_tail, std::memory_order_release ); // <BIND>
    // now the queue bindded again
  }

  inline Node* pop_head( ) 
  {
    assert( head_ && "<head> must always exist" );
    
    Node* const first = head_->next.load( std::memory_order_acquire ); // <READ>
    // synchronized with <BIND>
    if( !first ) {
      // there is no data or this queue is torn apart by some P-thread
      return nullptr;
    } else {
      delete head_;
      return head_ = first;
    }
  }

  Node*               head_; // pop elements from (used only by C-thread)
  std::atomic<Node*>  tail_; // push elements to
}; // class ScmpQueue

//--------------------------------------------------------------------------------
} // namespace concur
//--------------------------------------------------------------------------------

using concur::ScmpQueue; // for backward compatibility

//--------------------------------------------------------------------------------
# endif // _SCMP_QUEUE_H_

