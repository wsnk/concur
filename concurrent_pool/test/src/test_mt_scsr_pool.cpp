//--------------------------------------------------------------------------------
# include <boost/test/auto_unit_test.hpp>
# include <boost/test/test_tools.hpp>
# include "config.h"
# include "thread_helper.h"
//--------------------------------------------------------------------------------
# include <scsr_pool.h>
# include <scsp_ring_array.h>
//--------------------------------------------------------------------------------
# include <thread>
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE( SUITE_mt_scsr_pool )
//--------------------------------------------------------------------------------

typedef uint64_t item_type;

//--------------------------------------------------------------------------------

void run( )
{
  typedef concpool::ScsrPool< item_type >           pool_type;
  typedef concur::ScspRingArray< item_type*, 64 >   ring_type;
      
  const Config& cfg = get_config( );
  
  pool_type pool;
  pool.init( cfg.pool_capacity, sizeof( item_type ), 0 );
  
  ring_type ring;
  ring.init( cfg.pool_capacity );
  
  { // run threads
    ThreadHolder consumer;
    consumer.initialize( 1, [ & ]( unsigned ){
      for( uint64_t i( 0 ); i < cfg.iteration_count; ++i ) {
        item_type* it = nullptr;
        while( !( it = pool.pop( ) ) )
          ;
        
        while( !ring.push( it ) )
          ;
      }
    }, cfg.consumer_thread_affinity );
    
    ThreadHolder releaser;
    releaser.initialize( 1, [ & ]( unsigned ){
      for( int64_t i( 0 ); i < cfg.iteration_count; ++i ) {
        
        item_type* it = nullptr;
        while( !( ring.pop( it ) ) )
          ;
        
        ++( *it );
        pool.release( it );
      }
    }, cfg.releaser_thread_affinity );
    
    consumer.launch( );
    releaser.launch( );
  }
  
  {
    // the ring must be empty
    item_type* it = nullptr;
    BOOST_REQUIRE( !ring.pop( it ) );
  }
  
  { // check pool state
    uint64_t usage_count = 0;
    
    for( uint64_t i( 0 ); i < cfg.pool_capacity; ++i ) {
      item_type* it = pool.pop( );
      usage_count += *it;
      pool.release( it );
    }
    
    BOOST_CHECK( usage_count == cfg.iteration_count );
  }
}

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scsr_pool )
{
  run( );
} // CASE_scsr_pool
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END( ) // SUITE_mt_scsr_pool
//--------------------------------------------------------------------------------

