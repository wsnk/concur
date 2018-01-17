# ifndef _TEST_SCENE_1_H_
# define _TEST_SCENE_1_H_
//--------------------------------------------------------------------------------
# include <boost/test/test_tools.hpp>
//--------------------------------------------------------------------------------
# include "test_helpers.h"
//--------------------------------------------------------------------------------
# include <container_grab.h>
//--------------------------------------------------------------------------------
namespace scene_1 {
//--------------------------------------------------------------------------------
//
// Producers push items into given container. Consumers pop items and put them in
// corresponding vectors. After all these vectors are checked whether all produced
// values were received.
//
//--------------------------------------------------------------------------------

typedef counter_type                    element_type;
typedef Collectors< element_type >      collectors_type;

//--------------------------------------------------------------------------------

// default producer loop
template < typename ContainerT >
bool pexec( unsigned , void* , void* container_ptr, counter_type num )
{
  while( !static_cast< ContainerT* >( container_ptr )->push( num ) )
    ;
  return true;
}

// default consumer loop
template < typename ContainerT >
bool cexec( unsigned i, void* arg, void* container_ptr, counter_type )
{
  element_type dst{ };
  while( !static_cast< ContainerT* >( container_ptr )->pop( dst ) )
    ;
  static_cast< collectors_type* >( arg )->push( i, dst );
  return true;
}

// consumer loop for waitable container
template < typename ContainerT >
bool cexec_wait( unsigned i, void* arg, void* container_ptr, counter_type )
{
  element_type dst{ };
  while( !static_cast< ContainerT* >( container_ptr )->pop( dst, std::chrono::milliseconds( 100 ) ) )
    ;
  static_cast< collectors_type* >( arg )->push( i, dst );
  return true;
}

// producer loop through grab
template < typename ContainerT >
bool pexec_grab( unsigned , void* , void* container_ptr, counter_type num )
{
  ContainerT& container = *static_cast< ContainerT* >( container_ptr );
  while( !( concur::grab( container ).push( num ) ) )
    ;
  return true;
}

// consumer loop through grab
template < typename ContainerT >
bool cexec_grab( unsigned i, void* arg, void* container_ptr, counter_type )
{
  ContainerT& container = *static_cast< ContainerT* >( container_ptr );
  element_type dst{ };
  while( !( concur::grab( container ).pop( dst ) ) )
    ;
  static_cast< collectors_type* >( arg )->push( i, dst );
  return true;
}

//--------------------------------------------------------------------------------

// pointer producer loop
template < typename ContainerT >
bool pexec_ptr( unsigned , void* , void* container_ptr, counter_type num )
{
  typename ContainerT::element_type ptr = reinterpret_cast< typename ContainerT::element_type >( num + 1 );
  while( !static_cast< ContainerT* >( container_ptr )->push( ptr ) )
    ;
  
  return true;
}

// pointer consumer loop
template < typename ContainerT >
bool cexec_ptr( unsigned i, void* arg, void* container_ptr, counter_type )
{
  typename ContainerT::element_type ptr;
  while( !static_cast< ContainerT* >( container_ptr )->pop( ptr ) )
    ;
  
  static_cast< collectors_type* >( arg )->push( i, reinterpret_cast< element_type >( ptr ) - 1 );
  return true;
}

//--------------------------------------------------------------------------------

template < typename ContainerT >
struct ContainerTraits
{
  typedef ContainerT container_type;
  
  // producer function
  static SceneDesc::thread_func_type prod( ) {
    return &pexec< container_type >;
  }
  
  // consumer fucntion
  static SceneDesc::thread_func_type cons( ) {
    return &cexec< container_type >;
  }
  
  static void apply_to_scene( SceneDesc& ) { }
};

//--------------------------------------------------------------------------------

template < typename ContainerT >
void run_exchange_test( ContainerT& container, const char* test_name = "" )
{
  using namespace std::chrono;
  typedef ContainerTraits< ContainerT > traits;
  
  const TestConfig& cfg = get_config( );
  
  for( unsigned r( 0 ); r < cfg.repeat_count; ++r ) {
    collectors_type dests( cfg.cons_thread_count, cfg.operation_count );
    
    SceneDesc desc;
    desc.operations                 = cfg.operation_count;
    desc.prod_desc.function         = traits::prod( );
    desc.prod_desc.arg              = &dests;
    desc.prod_desc.thread_count     = cfg.prod_thread_count;
    desc.prod_desc.thread_affinity  = cfg.prod_thread_affinity;
    desc.cons_desc.function         = traits::cons( );
    desc.cons_desc.arg              = &dests;
    desc.cons_desc.thread_count     = cfg.cons_thread_count;
    desc.cons_desc.thread_affinity  = cfg.cons_thread_affinity;
    
    traits::apply_to_scene( desc );
    BOOST_TEST_MESSAGE( test_name << "("
                        << desc.prod_desc.thread_count << "/"
                        << desc.cons_desc.thread_count << ")" );
    
    
    CpuTimes times;
    
    run_scene( &container, desc, &times );
    
    BOOST_TEST_MESSAGE( "test '" << test_name << "' (" << r << ") took: " << times.str( ) );
    
    BOOST_CHECK( dests.check( 1, cfg.operation_count + 1 ) );
  }
}

//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
} // namespace scene_1
//--------------------------------------------------------------------------------
# endif // _TEST_SCENE_1_H_

