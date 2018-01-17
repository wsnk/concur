# include "thread_master.h"
# include <boost/test/test_tools.hpp>
//--------------------------------------------------------------------------------
# ifdef _WIN32
#   include <Windows.h>
#   include <WinBase.h>
# else
#   include <sched.h>
# endif
//--------------------------------------------------------------------------------
namespace {
//--------------------------------------------------------------------------------

# ifdef _WIN32
void set_thread_affinity( unsigned cpu )
{
  if( !SetThreadAffinityMask( GetCurrentThread( ), DWORD_PTR( 1 ) << cpu ) ) {
    DWORD dwError = GetLastError( );
    BOOST_TEST_MESSAGE( "failed to set affinity; error code: " << dwError );
  }
}
# else
void set_thread_affinity( unsigned cpu )
{
  cpu_set_t cpu_set;
  CPU_ZERO( &cpu_set );
  CPU_SET( cpu, &cpu_set );
  if( sched_setaffinity( 0, sizeof( cpu_set_t ), &cpu_set ) ) {
    BOOST_TEST_MESSAGE( "failed to set affinity; error code: " << errno );
  }
}
# endif
  
//--------------------------------------------------------------------------------
} // anonymous namespace
//--------------------------------------------------------------------------------
  

void ThreadMaster::initialize( const Config& cfg )
{
  cfg_ = cfg;
  threads_.reserve( cfg.count );
  for( unsigned i( 0 ); i < cfg.count; ++i ) {
    threads_.emplace_back( std::thread( [ this, i ]( ) {
      run( i );
    } ) );
  }
}

void ThreadMaster::run( unsigned num ) const
{
  if( cfg_.affinity >= 0 )
    set_thread_affinity( static_cast< unsigned >( cfg_.affinity ) + num );
  
  {
    std::unique_lock< std::mutex > lock( mtx_ );
    while( !started_ )
      cv_.wait( lock );
  }
  
  cfg_.func( num );
}

void ThreadMaster::launch( )
{
  {
    std::lock_guard< std::mutex > lock( mtx_ );
    started_ = true;
  }
  cv_.notify_all( );
}

void ThreadMaster::stop( )
{
  for( std::thread& t : threads_ ) {
    try { if( t.joinable( ) ) t.join( ); }
    catch( ... ) { }
  }
}
