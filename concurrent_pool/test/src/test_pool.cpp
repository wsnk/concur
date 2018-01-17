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
BOOST_AUTO_TEST_SUITE( SUITE_scmp_pool )
//--------------------------------------------------------------------------------

typedef int64_t                                 item_type;
typedef concpool::ScmrPool< item_type >         pool_type;
typedef pool_type::holder_type                  holder_type;
typedef concur::CondvarQueue< holder_type >     queue_type;

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_pool_cycle )
{
  pool_type pool;
  
  pool.create( 43 );
  
  // pool cycle ...
  for( unsigned i( 0 ); i < 10; ++i ) {
    holder_type holder = pool.pop( );
    BOOST_CHECK( holder && ( *holder == 43 ) );
    
    BOOST_CHECK( !( pool.pop( ) ) );
  }
} // CASE_pool_cycle
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END( ) // SUITE_scmp_pool
//--------------------------------------------------------------------------------
