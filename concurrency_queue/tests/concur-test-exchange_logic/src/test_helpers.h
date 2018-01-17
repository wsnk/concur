# ifndef _TEST_HELPERS_H_
# define _TEST_HELPERS_H_
//--------------------------------------------------------------------------------
# include <cassert>
# include <thread>
# include <atomic>
# include <condition_variable>
# include <chrono>
# include <vector>
# include <algorithm>
//--------------------------------------------------------------------------------

typedef int64_t                               counter_type;
typedef std::atomic< counter_type >           atomic_counter_type;
typedef std::chrono::steady_clock::duration   duration_type;

//--------------------------------------------------------------------------------

inline duration_type now( ) {
  return std::chrono::steady_clock::now( ).time_since_epoch( );
}

struct CpuTimes
{
  int_least64_t user;
  int_least64_t system;
  int_least64_t wall;
  
  std::string str( ) const;
};

//--------------------------------------------------------------------------------

void set_thread_affinity( unsigned cpu );

//--------------------------------------------------------------------------------

struct TestConfig
{
  unsigned  cons_thread_count       = 1;
  unsigned  prod_thread_count       = 1;
  unsigned  container_capacity      = 10000;
  int64_t   operation_count         = 1000000;
  int       prod_thread_affinity    = -1;
  int       cons_thread_affinity    = -1;
  unsigned  repeat_count            = 1;
};

const TestConfig& get_config( );

//--------------------------------------------------------------------------------

class ThreadHolder
{
public:
  ThreadHolder( const ThreadHolder& )               = delete;
  ThreadHolder& operator =( const ThreadHolder& )   = delete;
  
  ThreadHolder( ) : started_( false ), duration_( 0 ) { }
  ~ThreadHolder( ) { stop( ); }
  
  template < typename Func >
  void initialize( unsigned count, Func func, int affinity = -1 )
  {
    threads_.reserve( count );
    for( unsigned i( 0 ); i < count; ++i ) {
      threads_.emplace_back( std::thread( [ &, i, func, affinity ]( ) {
        if( affinity >= 0 )
          set_thread_affinity( static_cast< unsigned >( affinity ) + i );
        
        {
          std::unique_lock< std::mutex > lock( mtx_ );
          while( !started_ ) cv_.wait( lock );
        }
        
        const duration_type thread_start_ts = now( );
        func( i );
        const duration_type thread_dur = now( ) - thread_start_ts;
        
        {
          std::lock_guard< std::mutex > lock( mtx_ );
          if( thread_dur > duration_ ) duration_ = thread_dur;
        }
      } ) );
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
  
  duration_type stop( )
  {
    for( std::thread& t : threads_ ) {
      try {
        if( t.joinable( ) ) t.join( );
      } catch( ... ) { }
    }
    return duration_;
  }
  
private:
  std::vector< std::thread >  threads_;
  std::condition_variable     cv_;
  std::mutex                  mtx_;
  bool                        started_;
  duration_type               duration_;
}; // struct ThreadHolder

//--------------------------------------------------------------------------------

struct SceneDesc
{
  typedef bool( *thread_func_type )(
      unsigned,       // thread index
      void*,          // thread argument
      void*,          // exchange container
      counter_type    // operation number
    );
  
  struct ThreadDesc {
    thread_func_type  function        = nullptr;
    void*             arg             = nullptr;
    unsigned          thread_count    = 1;
    int               thread_affinity = -1;
    
  };
  
  ThreadDesc    prod_desc;
  ThreadDesc    cons_desc;
  counter_type  operations;
};

//--------------------------------------------------------------------------------

inline bool countdown( atomic_counter_type& cntr, counter_type& dst ) {
  if( ( dst = cntr.fetch_sub( 1 ) ) <= 0 ) {
    cntr.fetch_add( 1 );
    return false;
  }
  return true;
}

//--------------------------------------------------------------------------------

duration_type run_scene( void* queue, SceneDesc& desc, CpuTimes* times = nullptr );

//--------------------------------------------------------------------------------

template < typename Type >
class Collectors
{
public:
  Collectors( const Collectors& )               = delete;
  Collectors& operator =( const Collectors& )   = delete;
  
  Collectors( unsigned count, uint64_t reserve )
    : count_( count ), reserve_( reserve ), vectors_( new vector_type[ count ] )
  { while( count-- ) vectors_[ count ].reserve( reserve_ ); }
  
  ~Collectors( ) { delete[ ] vectors_; }
  
  template < typename T >
  inline void push( unsigned i, T&& val )
  {
    assert( i < count_ );
    vectors_[ i ].emplace_back( std::forward< T >( val ) );
  }
  
  bool check( const Type begin_val, const Type end_val )
  {
    vector_type sum;
    sum.reserve( reserve_ );
    
    for( unsigned i( 0 ); i < count_; ++i ) {
      for( std::size_t j( 0 ); j < vectors_[ i ].size( ); ++j )
        sum.push_back( vectors_[ i ][ j ] );
    }
    std::sort( sum.begin( ), sum.end( ) );
    
    
    auto  it  = sum.begin( );
    Type  val = begin_val; 
    while( it != sum.end( ) ) {
      if( *( it++ ) != ( val++ ) ) return false;
    }
    return val == end_val;
  }
  
private:
  typedef std::vector< Type > vector_type;
  
  unsigned      count_;
  uint64_t      reserve_;
  vector_type*  vectors_;
};

//--------------------------------------------------------------------------------
# endif // _TEST_HELPERS_H_


