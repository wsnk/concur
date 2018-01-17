//--------------------------------------------------------------------------------
# include <boost/test/auto_unit_test.hpp>
# include <boost/test/test_tools.hpp>
# include "config.h"
# include "thread_helper.h"
//--------------------------------------------------------------------------------
# include <scmr_pool.h>
//--------------------------------------------------------------------------------
# include <condvar_queue.h>
//--------------------------------------------------------------------------------
# include <thread>
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE( SUITE_scmp_pool_mt )
//--------------------------------------------------------------------------------

void log_progress( const char* prefix, double progress )
{
  char str[256];
  std::sprintf( str, "%s: %10.2f %% done", prefix, progress );
  BOOST_TEST_MESSAGE( str );
}

//--------------------------------------------------------------------------------

typedef uint64_t item_type;

//--------------------------------------------------------------------------------

void run( )
{
  typedef concpool::ScmrPool< item_type >             pool_type;
  typedef pool_type::holder_type                      holder_type;
  typedef concur::CondvarQueue< holder_type >         queue_type;
      
  Config cfg = get_config( );
  
  pool_type   pool;
  queue_type  queue;
  
  // initialize the pool
  for( int64_t i( 0 ); i < cfg.pool_capacity; ++i )
    pool.create( 0 );
  
  const uint64_t total_count  = cfg.iteration_count * cfg.releaser_thread_count;
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
    rthreads.initialize( cfg.releaser_thread_count, [ & ]( unsigned ){
      for( int64_t i( 0 ); i < cfg.iteration_count; ++i ) {
        holder_type holder;
        while( !queue.pop( holder, std::chrono::seconds( 1 ) ) ) ;
        
        ++( *holder );
      }
    }, cfg.releaser_thread_affinity );
    
    start_time = std::chrono::steady_clock::now( ).time_since_epoch( );
    cthreads.launch( );
    rthreads.launch( );
  }
  std::chrono::nanoseconds duration = std::chrono::steady_clock::now( ).time_since_epoch( ) - start_time;
  BOOST_TEST_MESSAGE( "duration: " << ( double( duration.count( ) ) / 1000000000.0 ) << "sec." );
  
  { // check pool state
    uint64_t  elements_count  = 0;
    uint64_t  usage_count     = 0;
    
    holder_type holder;
    while( holder = pool.pop( ) ) {
      elements_count  += 1;
      usage_count     += *holder;
      queue.push( std::move( holder ) );
    }
    
    BOOST_CHECK( elements_count == cfg.pool_capacity );
    BOOST_CHECK( usage_count == total_count );
  }
}

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_pool_cycle )
{
  run( );
} // CASE_pool_cycle
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END( ) // SUITE_scmp_pool_mt
//--------------------------------------------------------------------------------
