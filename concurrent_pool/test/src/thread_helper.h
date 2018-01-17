# ifndef _THREAD_HELPER_H_
# define _THREAD_HELPER_H_
//--------------------------------------------------------------------------------
# include <thread>
# include <condition_variable>
# include <vector>
//--------------------------------------------------------------------------------
# include <boost/test/test_tools.hpp>
//--------------------------------------------------------------------------------
# ifdef _WIN32
#   include <Windows.h>
#   include <WinBase.h>

  inline void set_thread_affinity( unsigned cpu )
  {
    if( !SetThreadAffinityMask( GetCurrentThread( ), DWORD_PTR( 1 ) << cpu ) ) {
      DWORD dwError = GetLastError( );
      BOOST_TEST_MESSAGE( "failed to set affinity; error code: " << dwError );
    }
  }
# else
#   include <sched.h>

  inline void set_thread_affinity( unsigned cpu )
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
  
class ThreadHolder
{
public:
  ThreadHolder( const ThreadHolder& )               = delete;
  ThreadHolder& operator =( const ThreadHolder& )   = delete;
  
  ThreadHolder( ) : started_( false ) { }
  ~ThreadHolder( ) { stop( ); }
  
  template < typename Func >
  void initialize( unsigned count, Func func, int affinity = -1 )
  {
    threads_.reserve( count );
    for( unsigned i( 0 ); i < count; ++i ) {
      threads_.emplace_back(
            std::thread( [ this, i, func, affinity ]( ) { run( i, func, affinity ); } )
            );
    }
  }
  
  void launch( )
  {
    {
      std::unique_lock< std::mutex > lock( mtx_ );
      started_ = true;
    }
    cv_.notify_all( );
  }
  
  void stop( )
  {
    for( std::thread& t : threads_ ) {
      try { if( t.joinable( ) ) t.join( ); }
      catch( ... ) { }
    }
  }
  
private:
  template < typename Func >
  void run( unsigned num, Func func, int affinity )
  {
    if( affinity >= 0 )
      set_thread_affinity( static_cast< unsigned >( affinity ) + num );
    
    {
      std::unique_lock< std::mutex > lock( mtx_ );
      while( !started_ )
        cv_.wait( lock );
    }
    
    func( num );
  }
  
private:
  std::vector< std::thread >  threads_;
  std::condition_variable     cv_;
  std::mutex                  mtx_;
  bool                        started_;
}; // struct ThreadHolder

//--------------------------------------------------------------------------------
#endif // THREAD_HELPER_H

