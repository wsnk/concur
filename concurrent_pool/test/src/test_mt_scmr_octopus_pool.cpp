//--------------------------------------------------------------------------------
# include <boost/test/auto_unit_test.hpp>
# include <boost/test/test_tools.hpp>
# include "config.h"
# include "thread_helper.h"
//--------------------------------------------------------------------------------
# include <scmr_octopus_pool.h>
//--------------------------------------------------------------------------------
# include <scmp_ring_array.h>
//--------------------------------------------------------------------------------
# include <thread>
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE( SUITE_mt_scmr_octopus_pool )
//--------------------------------------------------------------------------------

typedef uint64_t item_type;

//--------------------------------------------------------------------------------

void run( )
{
  typedef concpool::ScmrOctopusPool< item_type >    pool_type;
  typedef concur::ScmpRingArray< item_type*, 64 >   ring_type;
      
  const Config& cfg = get_config( );
  
  pool_type pool;
  pool.init( cfg.releaser_thread_count, cfg.pool_capacity, sizeof( item_type ), 0 );
  
  ring_type* rings = new ring_type[ cfg.releaser_thread_count ];
  for( unsigned i( 0 ); i < cfg.releaser_thread_count; ++i )
    rings[ i ].init( cfg.pool_capacity );
  
  const uint64_t total_count = cfg.iteration_count * cfg.releaser_thread_count;
  bool consumer_check = true;
  bool releaser_check = true;
  
  { // run threads
    ThreadHolder consumer;
    consumer.initialize( 1, [ & ]( unsigned ){
      for( uint64_t i( 0 ); i < total_count; ++i ) {
        const unsigned rnum = i % cfg.releaser_thread_count;
        
        item_type* it = nullptr;
        while( !( it = pool.pop( ) ) )
          ;
        
        consumer_check &= ( *it == 0 );
        *it = rnum;
        
        while( !rings[ rnum ].push( it ) )
          ;
      }
    }, cfg.consumer_thread_affinity );
    
    ThreadHolder releasers;
    releasers.initialize( cfg.releaser_thread_count, [ & ]( unsigned rnum ){
      ring_type& ring = rings[ rnum ];
      for( int64_t i( 0 ); i < cfg.iteration_count; ++i ) {
        item_type* it = nullptr;
        while( !( ring.pop( it ) ) )
          ;
        
        releaser_check &= ( *it == rnum );
        *it = 0;

        pool.release( rnum, it );
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
  
  BOOST_CHECK( consumer_check );
  BOOST_CHECK( releaser_check );
}

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scmr_octopus_pool )
{
  run( );
} // CASE_scmr_octopus_pool
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END( ) // SUITE_mt_scmr_octopus_pool
//--------------------------------------------------------------------------------
