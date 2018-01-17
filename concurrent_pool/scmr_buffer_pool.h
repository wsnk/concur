# ifndef _SCMR_BUFFER_POOL_H_
# define _SCMR_BUFFER_POOL_H_
//--------------------------------------------------------------------------------
# include <atomic>
# include <stdexcept>
//--------------------------------------------------------------------------------
namespace concpool {
//--------------------------------------------------------------------------------
namespace details {
//--------------------------------------------------------------------------------

struct NodeHeader
{
  void*                       pool;
  std::atomic< NodeHeader* >  next;
  
  NodeHeader( void* pool_ptr ) : pool( pool_ptr ), next( nullptr ) { }
};

NodeHeader* create_node( void* pool_ptr, std::size_t payload_size )
{
  void* mem = std::malloc( sizeof( NodeHeader ) + payload_size );
  if( !mem )
    throw std::bad_alloc( );
  return new( mem ) NodeHeader( pool_ptr );
}

void free_node_chain( NodeHeader* begin )
{
  while( begin ) {
    NodeHeader* next = begin->next.load( std::memory_order_relaxed );
    std::free( begin );
    begin = next;
  }
}

//--------------------------------------------------------------------------------
} // namespace details
//--------------------------------------------------------------------------------

class ScmrBufferPool
{
public:
  ScmrBufferPool( const ScmrBufferPool& )             = delete;
  ScmrBufferPool& operator =( const ScmrBufferPool& ) = delete;
  
  static void release( void* ptr )
  {
    assert( ptr );
    
    details::NodeHeader* hdr = static_cast< details::NodeHeader* >( ptr ) - 1;
    static_cast< ScmrBufferPool* >( hdr->pool )->push_node( hdr );
  }
  
public:
  ScmrBufferPool( ) : tail_( nullptr ), head_( nullptr ) { }
  ~ScmrBufferPool( ) { details::free_node_chain( tail_ ); }
  
  void init( std::size_t count, std::size_t payload_size )
  {
    if( tail_ || head_.load( ) )
      throw std::logic_error( "pool already initialized" );
    
    head_.store( tail_ = details::create_node( this, payload_size ) );
    while( count-- )
      push_node( details::create_node( this, payload_size ) );
  }
  
  inline void* pop( )
  {
    node_type* node = pop_node( );
    return node ? ( node + 1 ) : nullptr;
  }

private:
  typedef details::NodeHeader node_type;
  
  void push_node( node_type* node ) 
  {
    assert( node && ( node->pool == this ) && ( node->next.load( ) == nullptr ) );
    
    node_type* old_head = head_.exchange( node, std::memory_order_acq_rel );
    old_head->next.store( node, std::memory_order_release );
  }

  node_type* pop_node( ) 
  {
    node_type* node = tail_->next.exchange( nullptr, std::memory_order_acquire );
    if( node )
      std::swap( tail_, node );
    return node;
  }

private:
  node_type*                 tail_; // pop elements from
  std::atomic< node_type* >  head_; // release elements to
}; // class ScmrBufferPool

//--------------------------------------------------------------------------------
} // namespace concpool
//--------------------------------------------------------------------------------
# endif // _SCMR_BUFFER_POOL_H_

