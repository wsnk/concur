//--------------------------------------------------------------------------------
# include <boost/test/auto_unit_test.hpp>
# include <boost/test/test_tools.hpp>
# include "config.h"
# include "thread_helper.h"
//--------------------------------------------------------------------------------
# include <scsp_ring_array.h>
//--------------------------------------------------------------------------------
# include "scmr_buffer_pool.h"
# include "scmr_ring_pool.h"
# include "scmr_octopus_pool.h"
# include <scmp_ring_array.h>
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE( SUITE_scmr_buffer_pool )
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
namespace {
//--------------------------------------------------------------------------------

typedef concur::ScspRingArray< void* >  ring_type;      // producer -> releaser
typedef std::chrono::nanoseconds        nanosec_type;
  
inline nanosec_type now( ) {
  return std::chrono::steady_clock::now( ).time_since_epoch( );
}

const char DATA[ ] = "PEN-PINEAPPLE-APPLE-PEN";

//--------------------------------------------------------------------------------

template < typename PoolType > class Producer
{
public:
  Producer( PoolType& pool, ring_type* rings, unsigned rcount )
    : pool_( pool ), rings_( rings ), rcount_( rcount )
  { }
  
  void operator( )( uint64_t count ) const
  {
    for( uint64_t i( 0 ); i < count; ++i ) {
      void* buff = nullptr;
      while( !( buff = pool_.pop( ) ) )
        ;
      
      ring_type& ring = rings_[ i % rcount_ ];
      while( !ring.push( buff ) )
        ;
    }
  }
  
private:
  PoolType&     pool_;
  ring_type*    rings_;
  unsigned      rcount_;
}; // class Producer
  
//--------------------------------------------------------------------------------
  
template < typename PoolType >
class Releaser
{
public:
  Releaser( PoolType& pool, ring_type& ring, unsigned num )
    : pool_( pool ), ring_( ring ), num_( num )
  { }
  
  void operator( )( uint64_t count ) const
  {
    for( uint64_t i( 0 ); i < count; ++i ) {
      void* buff = nullptr;
      while( !ring_.pop( buff ) )
        ;
      
      pool_.release( num_, buff );
    }
  }
  
private:
  PoolType&   pool_;
  ring_type&  ring_;
  unsigned    num_;
}; // class Releaser
  
//--------------------------------------------------------------------------------
  
template < typename PoolType >
void run( const char* title )
{
  Config cfg = get_config( );
  
  const unsigned buffer_size        = 256;
  const unsigned ring_capacity      = cfg.pool_capacity;
  const unsigned releaser_count     = cfg.releaser_thread_count;
  const uint64_t per_releaser_count = cfg.iteration_count;
  const uint64_t total_count        = cfg.iteration_count * releaser_count;
  
  for( unsigned i( 0 ); i < get_config( ).repeat_count; ++i ) {
    // init pool
    PoolType pool;
    pool.init( releaser_count, cfg.pool_capacity, buffer_size );
    
    // init rings
    ring_type* rings = new ring_type[ releaser_count ];
    for( unsigned i( 0 ); i < releaser_count; ++i )
      rings[ i ].init( ring_capacity );
    
    nanosec_type start_time;
    
    { // run threads
      ThreadHolder producer;
      producer.initialize( 1, [ & ]( unsigned ){
        Producer< PoolType > p( pool, rings, releaser_count );
        p( total_count );
      }, cfg.consumer_thread_affinity );
      
      ThreadHolder releasers;
      releasers.initialize( releaser_count, [ & ]( unsigned num ){
        Releaser< PoolType > r( pool, rings[ num ], num );
        r( per_releaser_count );
      }, cfg.releaser_thread_affinity );
      
      BOOST_TEST_MESSAGE( title << ": started..." );
      start_time = now( );
      producer.launch( );
      releasers.launch( );
    }
    
    const double dur = ( now( ) - start_time ).count( ) / 1000000000.0;
    BOOST_TEST_MESSAGE( title << ": completed. duration " << dur << " sec." );
    
    delete[ ] rings;
  }
}
  
//--------------------------------------------------------------------------------
} // anonymous namespace
//--------------------------------------------------------------------------------

struct ScmrBufferPoolWrap
{
  inline void init( unsigned, std::size_t count, std::size_t buffer_size ) {
    pool.init( count, buffer_size );
  }
  
  inline void release( unsigned , void* buff ) {
    pool.release( buff );
  }
  
  inline void* pop( ) {
    return pool.pop( );
  }
  
private:
  concpool::ScmrBufferPool pool;
};

BOOST_AUTO_TEST_CASE( CASE_scmr_buffer_pool )
{
  run< ScmrBufferPoolWrap >( "ScmrBufferPool" );
}

//--------------------------------------------------------------------------------

struct ScmpRingWrap
{
  ~ScmpRingWrap( )
  {
    void* buff = nullptr;
    while( ring.pop( buff ) )
      std::free( buff );
  }

  inline void init( unsigned, std::size_t count, std::size_t buffer_size )
  {
    // init ring
    ring.init( count );
    
    // allocate buffers
    while( count-- ) {
      void* buff = std::malloc( buffer_size );
      if( !ring.push( buff ) ) throw std::runtime_error( "TEST ERROR" );
    }
  }
  
  inline void release( unsigned, void* buff )
  {
    ring.push( buff );
  }
  
  inline void* pop( )
  {
    void* buff = nullptr;
    ring.pop( buff );
    return buff;
  }
  
  concur::ScmpRingArray< void*, 64 >  ring;
};

BOOST_AUTO_TEST_CASE( CASE_scmp_ring )
{
  run< ScmpRingWrap >( "ScmpRing" );
}

//--------------------------------------------------------------------------------

struct ScmrRingPoolWrap
{
  inline void init( unsigned, std::size_t count, std::size_t buffer_size ) {
    pool.init( count, buffer_size );
  }
  
  inline void release( unsigned, void* buff ) {
    pool.release( static_cast< BufferHeader* >( buff ) );
  }
  
  inline void* pop( ) {
    return pool.pop( );
  }
  
private:
  struct BufferHeader { };
  concpool::ScmrRingPool< BufferHeader, 64 >  pool;
};

BOOST_AUTO_TEST_CASE( CASE_scmr_ring_pool )
{
  run< ScmrRingPoolWrap >( "ScmrRingPool" );
}

//--------------------------------------------------------------------------------

template < unsigned Alignment = 64 >
struct ScmrOctopusPoolWrap
{
  inline void init( unsigned rcount, std::size_t count, std::size_t buffer_size ) {
    pool.init( rcount, count, buffer_size );
  }
  
  inline void release( unsigned rnum, void* buff ) {
    pool.release( rnum, static_cast< BufferHeader* >( buff ) );
  }
  
  inline void* pop( ) {
    return pool.pop( );
  }
  
private:
  struct BufferHeader { };
  concpool::ScmrOctopusPool< BufferHeader, Alignment >  pool;
};

BOOST_AUTO_TEST_CASE( CASE_scmr_octopus_pool )
{
  run< ScmrOctopusPoolWrap< 1 > >( "ScmrOctopusPool<0>" );
  run< ScmrOctopusPoolWrap< 32 > >( "ScmrOctopusPool<32>" );
  run< ScmrOctopusPoolWrap< 64 > >( "ScmrOctopusPool<64>" );
  run< ScmrOctopusPoolWrap< 128 > >( "ScmrOctopusPool<128>" );
}

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END( ) // SUITE_scmr_buffer_pool
//--------------------------------------------------------------------------------



