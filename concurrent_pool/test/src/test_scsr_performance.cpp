//--------------------------------------------------------------------------------
# include <boost/test/auto_unit_test.hpp>
# include <boost/test/test_tools.hpp>
# include "config.h"
# include "thread_helper.h"
//--------------------------------------------------------------------------------
# include <scmr_pool.h>
//--------------------------------------------------------------------------------
# include <scsp_ring_array.h>
# include <scmp_ring_array.h>
//--------------------------------------------------------------------------------
# include <thread>
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE( SUITE_scsr_performance )
//--------------------------------------------------------------------------------

typedef uint64_t item_type;

//--------------------------------------------------------------------------------

void run( )
{
  typedef concpool::ScmrPool< item_type >             pool_type;
  typedef pool_type::holder_type                      holder_type;
  typedef concur::ScspRingArray< holder_type >        queue_type;
      
  Config cfg = get_config( );
  
  pool_type pool;
  for( int64_t i( 0 ); i < cfg.pool_capacity; ++i )
    pool.create( 0 );
  
  queue_type  queue;
  queue.init( cfg.pool_capacity  * 2);
  
  const uint64_t total_count  = cfg.iteration_count;
  std::chrono::nanoseconds start_time;
  
  { // run threads
    ThreadHolder cthreads;
    cthreads.initialize( 1, [ & ]( unsigned ){
      for( uint64_t i( 0 ); i < total_count; ++i ) {
        holder_type holder;
        
        while( !( holder = pool.pop( ) ) ) ;
        queue.push( std::move( holder ) );
      }
    }, cfg.releaser_thread_affinity );
    
    ThreadHolder rthreads;
    rthreads.initialize( 1, [ & ]( unsigned ){
      for( int64_t i( 0 ); i < cfg.iteration_count; ++i ) {
        holder_type holder;
        
        while( !queue.pop( holder ) ) ;
        ++( *holder );
      }
    }, cfg.releaser_thread_affinity );
    
    start_time = std::chrono::steady_clock::now( ).time_since_epoch( );
    cthreads.launch( );
    rthreads.launch( );
  }
  std::chrono::nanoseconds duration = std::chrono::steady_clock::now( ).time_since_epoch( ) - start_time;
  BOOST_TEST_MESSAGE( "[SCMRPool/SCSPRing] duration: " << ( double( duration.count( ) ) / 1000000000.0 ) << "sec." );
}

//--------------------------------------------------------------------------------

void run_ring( )
{
  typedef concur::ScmpRingArray< item_type, 64 >      ring_type;
  typedef concur::ScspRingArray< item_type >          queue_type;
      
  Config cfg = get_config( );
  
  ring_type pool;
  pool.init( cfg.pool_capacity );
  for( int64_t i( 0 ); i < cfg.pool_capacity; ++i )
    pool.push( 0 );
  
  queue_type  queue;
  queue.init( cfg.pool_capacity  * 2);
  
  const uint64_t total_count  = cfg.iteration_count;
  std::chrono::nanoseconds start_time;
  
  { // run threads
    ThreadHolder cthreads;
    cthreads.initialize( 1, [ & ]( unsigned ){
      for( uint64_t i( 0 ); i < total_count; ++i ) {
        
        item_type dst;
        while( !pool.pop( dst ) ) ;
        
        queue.push( std::move( dst ) );
      }
    }, cfg.releaser_thread_affinity );
    
    ThreadHolder rthreads;
    rthreads.initialize( 1, [ & ]( unsigned ){
      for( int64_t i( 0 ); i < cfg.iteration_count; ++i ) {
        item_type dst;
        while( !queue.pop( dst ) ) ;
        
        ++dst;
        pool.push( std::move( dst ) );
      }
    }, cfg.releaser_thread_affinity );
    
    start_time = std::chrono::steady_clock::now( ).time_since_epoch( );
    cthreads.launch( );
    rthreads.launch( );
  }
  std::chrono::nanoseconds duration = std::chrono::steady_clock::now( ).time_since_epoch( ) - start_time;
  BOOST_TEST_MESSAGE( "[SCMPRing/SCSPRing] duration: " << ( double( duration.count( ) ) / 1000000000.0 ) << "sec." );
}

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_pool_cycle )
{
  run( );
} // CASE_pool_cycle
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_ring_cycle )
{
  run_ring( );
} // CASE_ring_cycle
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END( ) // SUITE_scmp_pool_mt
//--------------------------------------------------------------------------------
