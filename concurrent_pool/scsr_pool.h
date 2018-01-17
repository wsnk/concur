# ifndef _SCSR_POOL_H_
# define _SCSR_POOL_H_
//--------------------------------------------------------------------------------
# include <cassert>
# include <cstdlib>
# include <stdexcept>
# include <atomic>
# include <utils/mem_utils.h>
//--------------------------------------------------------------------------------
namespace concpool {
//--------------------------------------------------------------------------------

template < typename Type, unsigned Alignment = 64 >
class ALIGNAS( Alignment ) ScsrPool
{
public:
  typedef Type element_type;
  
  ScsrPool( const ScsrPool& )             = delete;
  ScsrPool& operator =( const ScsrPool& ) = delete;
  
public:
  ScsrPool( ) : tail_( nullptr ), head_( nullptr ) { }
  ~ScsrPool( ) { destroy_node_chain( tail_ ); }
  
  template < typename ...Args >
  void init( std::size_t count, std::size_t element_size, Args ...args )
  {
    head_ = tail_ = create_node( element_size, args... );
    while( --count )
      push_node( create_node( element_size, args... ) );
  }
  
  inline element_type* pop( )
  {
    Node* node = pop_node( );
    return node ? to_element( node ) : nullptr;
  }
  
  inline void release( element_type* ptr )
  {
    push_node( to_node( ptr ) );
  }

private:
  struct ALIGNAS( Alignment ) Node
  {
    std::atomic< Node* > next;
    
    Node( ) : next( nullptr ) { }
  };
  
  static inline element_type* to_element( Node* node )
  {
    return reinterpret_cast< element_type* >( node + 1 );
  }
  static inline Node* to_node( element_type* element )
  {
    return reinterpret_cast< Node* >( element ) - 1;
  }
  
  template < typename ...Args >
  static Node* create_node( std::size_t element_size, Args ...args )
  {
    void* ptr = aligned_malloc( Alignment, sizeof( Node ) + element_size );
    if( !ptr )
      throw std::bad_alloc( );
    new( ( char* )ptr + sizeof( Node ) ) element_type( args... );
    return new( ptr ) Node;
  }
  
  static void destroy_node_chain( Node* begin )
  {
    while( begin ) {
      to_element( begin )->~element_type( );
      Node* next = begin->next.load( std::memory_order_relaxed );
      aligned_free( begin );
      begin = next;
    }
  }
  
  void push_node( Node* node ) 
  {
    assert( node->next.load( ) == nullptr );
    head_->next.store( node, std::memory_order_release );
    head_ = node;
  }

  Node* pop_node( ) 
  {
    Node* node = tail_->next.exchange( nullptr, std::memory_order_acquire );
    if( node )
      std::swap( tail_, node );
    return node;
  }

private:
  Node*  tail_;
  Node*  head_;
}; // class ScsrPool

//--------------------------------------------------------------------------------
} // namespace concpool
//--------------------------------------------------------------------------------
#endif // SCSR_POOL_H

