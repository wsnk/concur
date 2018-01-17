//--------------------------------------------------------------------------------
# include <boost/test/auto_unit_test.hpp>
# include <boost/test/test_tools.hpp>
# include <memory>
//--------------------------------------------------------------------------------
# include "test_config.h"
# include "thread_master.h"
//--------------------------------------------------------------------------------
# include <scmp_ring_collection.h>
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE( SUITE_scmp_ring_collection )
//--------------------------------------------------------------------------------

typedef uint64_t item_type;

//--------------------------------------------------------------------------------
// static checks

static_assert( sizeof( concur::ScmpRingCollection< item_type, 64 > ) == 128, "invalid ring layout" );

//--------------------------------------------------------------------------------

void run( )
{
  typedef concur::ScmpRingCollection< item_type, 64 >   ring_type;
  typedef std::vector< item_type >                      receiver_type;
  const Config& cfg = get_config( );
  
  ring_type ring;
  ring.init( cfg.container_capacity );
  
  std::unique_ptr< receiver_type[ ] > recs( new receiver_type[ cfg.prod_thread_count ] );
  for( unsigned i( 0 ); i < cfg.prod_thread_count; ++i )
    recs[ i ].reserve( cfg.operation_count );
  
  {
    ThreadMaster consumer;
    ThreadMaster producer;
    
    {
      ThreadMaster::Config thread_cfg;
      thread_cfg.affinity = cfg.prod_thread_affinity;
      thread_cfg.count    = cfg.prod_thread_count;
      thread_cfg.func     = [ & ]( unsigned num ) {
        const uint64_t id = static_cast< uint64_t >( num ) << 56;
        for( uint64_t i = 0; i < cfg.operation_count; ++i ) {
          item_type* dst = nullptr;
          while( !( dst = ring.producer_fetch( ) ) )
            ;
          
          *dst = id | i;
          
          ring.producer_release( dst );
        }
      };
      producer.initialize( thread_cfg );
    } {
      ThreadMaster::Config thread_cfg;
      thread_cfg.affinity = cfg.cons_thread_affinity;
      thread_cfg.count    = cfg.cons_thread_count;
      thread_cfg.func     = [ & ]( unsigned ) {
        const uint64_t total = cfg.operation_count * cfg.prod_thread_count;
        for( uint64_t i = 0; i < total; ++i ) {
          item_type* ptr = nullptr;
          while( !( ptr = ring.consumer_fetch( ) ) )
            ;
          
          const uint64_t val = *ptr;
          ring.consumer_release( ptr );
          
          const unsigned prod_num = static_cast< unsigned >( val >> 56 );
          recs[ prod_num ].push_back( val & 0x00FFFFFFFFFFFFFFull );
        }
      };
      consumer.initialize( thread_cfg );
    }
    
    producer.launch( );
    consumer.launch( );
  }
  
  // check
  for( unsigned num( 0 ); num < cfg.prod_thread_count; ++num ) {
    receiver_type& rec = recs[ num ];
    BOOST_REQUIRE( rec.size( ) == cfg.operation_count );
    for( uint64_t i( 0 ); i < cfg.operation_count; ++i ) {
      BOOST_CHECK( rec[ i ] == i );
    }
  }
}

BOOST_AUTO_TEST_CASE( CASE_basic )
{
  typedef concur::ScmpRingCollection< item_type, 64 > ring_type;
  
  ring_type ring;
  ring.init( 1000 );
  
  for( unsigned i( 0 ); i < 1000; ++i )
    ring.at( i ) = i;
  
  for( unsigned i( 0 ); i < 1000; ++i )
    BOOST_CHECK( ring.at( i ) == i );
}

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scmp_ring_collection )
{
  run( );
} // CASE_scmp_ring_collection
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END( ) // SUITE_scmp_ring_collection
//--------------------------------------------------------------------------------
