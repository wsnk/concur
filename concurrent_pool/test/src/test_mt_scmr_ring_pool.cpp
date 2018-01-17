//--------------------------------------------------------------------------------
# include <boost/test/auto_unit_test.hpp>
# include <boost/test/test_tools.hpp>
# include "config.h"
# include "thread_helper.h"
//--------------------------------------------------------------------------------
# include <scmr_ring_pool.h>
//--------------------------------------------------------------------------------
# include <scmp_ring_array.h>
//--------------------------------------------------------------------------------
# include <thread>
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE( SUITE_mt_scmr_ring_pool )
//--------------------------------------------------------------------------------

typedef uint64_t item_type;

//--------------------------------------------------------------------------------

void run( )
{
  typedef concpool::ScmrRingPool< item_type >   pool_type;
  typedef concur::ScmpRingArray< item_type*, 64 >   ring_type;
      
  const Config& cfg = get_config( );
  
  pool_type pool;
  pool.init( cfg.pool_capacity, sizeof( item_type ), 0 );
  
  ring_type* rings = new ring_type[ cfg.releaser_thread_count ];
  for( unsigned i( 0 ); i < cfg.releaser_thread_count; ++i )
    rings[ i ].init( cfg.pool_capacity );
  
  const uint64_t total_count = cfg.iteration_count * cfg.releaser_thread_count;
  
  { // run threads
    ThreadHolder consumer;
    consumer.initialize( 1, [ & ]( unsigned ){
      for( uint64_t i( 0 ); i < total_count; ++i ) {
        
        item_type* it = nullptr;
        while( !( it = pool.pop( ) ) )
          ;
        
        ring_type& ring = rings[ i % cfg.releaser_thread_count ];
        while( !ring.push( it ) )
          ;
      }
    }, cfg.consumer_thread_affinity );
    
    ThreadHolder releasers;
    releasers.initialize( cfg.releaser_thread_count, [ & ]( unsigned i ){
      ring_type& ring = rings[ i ];
      for( int64_t i( 0 ); i < cfg.iteration_count; ++i ) {
        
        item_type* it = nullptr;
        while( !( ring.pop( it ) ) )
          ;
        
        ++( *it );
        pool.release( it );
      }
    }, cfg.releaser_thread_affinity );
    
    consumer.launch( );
    releasers.launch( );
  }
  
  {
    // all rings must be empty
    item_type* it = nullptr;
    for( unsigned i( 0 ); i < cfg.releaser_thread_count; ++i ) 
      BOOST_REQUIRE( !rings[ i ].pop( it ) );
    
    delete[ ] rings;
  }
  
  { // check pool state
    uint64_t  elements_count  = 0;
    uint64_t  usage_count     = 0;
    
    std::vector< item_type* > holder;
    holder.reserve( total_count );
    
    // take all items from the pool
    while( item_type* it = pool.pop( ) ) {
      holder.push_back( it );
      elements_count  += 1;
      usage_count     += *it;
    }
    
    // return all items to the pool
    for( item_type* it : holder ) pool.release( it );
    
    BOOST_CHECK( elements_count == cfg.pool_capacity );
    BOOST_CHECK( usage_count    == total_count );
  }
}

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scmr_ring_pool )
{
  run( );
} // CASE_scmr_ring_pool
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END( ) // SUITE_mt_scmr_ring_pool
//--------------------------------------------------------------------------------
