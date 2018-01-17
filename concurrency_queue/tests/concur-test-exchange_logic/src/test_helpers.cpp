# include "test_helpers.h"
//--------------------------------------------------------------------------------
# ifdef _WIN32
#   include <Windows.h>
#   include <WinBase.h>
# else
#   include <sched.h>
# endif
//--------------------------------------------------------------------------------
# include <boost/test/test_tools.hpp>
# include <boost/timer/timer.hpp>
//--------------------------------------------------------------------------------

std::string CpuTimes::str( ) const
{
  const double user   = this->user    / 1000000000.0;
  const double system = this->system  / 1000000000.0;
  const double sum    = user + system;
  const double wall   = this->wall    / 1000000000.0;
  
  char buff[512];
  
  int written = std::sprintf( buff, "usr: %10f; sys: %10f; sum: %10f; wll: %10f",
                              user, system, sum, wall
                              );
  
  if( written < 0 )
    return "failed";
  return std::string( buff, written );
}

//--------------------------------------------------------------------------------

void set_thread_affinity( unsigned cpu )
{
# ifdef _WIN32
  if( !SetThreadAffinityMask( GetCurrentThread( ), DWORD_PTR( 1 ) << cpu ) ) {
    DWORD dwError = GetLastError( );
    BOOST_TEST_MESSAGE( "failed to set affinity; error code: " << dwError );
  }
# else
  cpu_set_t cpu_set;
  CPU_ZERO( &cpu_set );
  CPU_SET( cpu, &cpu_set );
  if( sched_setaffinity( 0, sizeof( cpu_set_t ), &cpu_set ) ) {
    BOOST_TEST_MESSAGE( "failed to set affinity; error code: " << errno );
  }
# endif
}

//--------------------------------------------------------------------------------

duration_type run_scene( void* container, SceneDesc& desc, CpuTimes* times )
{
  assert( container != nullptr );
  
  atomic_counter_type prod_counter( desc.operations );
  atomic_counter_type cons_counter( desc.operations );
  
  duration_type duration( 0 );
  
  {
    ThreadHolder pholder;
    if( desc.prod_desc.function ) {
      pholder.initialize( desc.prod_desc.thread_count, [ & ]( unsigned i ) {
        counter_type num( 0 );
        while( countdown( prod_counter, num ) &&
               desc.prod_desc.function( i, desc.prod_desc.arg, container, num ) )
          ;
      }, desc.prod_desc.thread_affinity );
    }
    
    ThreadHolder cholder;
    if( desc.cons_desc.function ) {
      cholder.initialize( desc.cons_desc.thread_count, [ & ]( unsigned i ) {
        counter_type num( 0 );
        while( countdown( cons_counter, num ) &&
               desc.cons_desc.function( i, desc.cons_desc.arg, container, num ) )
          ;
      }, desc.cons_desc.thread_affinity );
    }
    
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    
    boost::timer::cpu_timer timer;
    
    pholder.launch( );
    cholder.launch( );
    
    // ... processing
    
    const duration_type pdur = pholder.stop( );
    const duration_type cdur = pholder.stop( );
    
    timer.stop( );
    if( times ) {
      times->user   = timer.elapsed( ).user;
      times->system = timer.elapsed( ).system;
      times->wall   = timer.elapsed( ).wall;
    }
    
    duration = ( pdur > cdur ) ? pdur : cdur;
  }
  
  return duration;
}

