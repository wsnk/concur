//--------------------------------------------------------------------------------
# include <boost/test/auto_unit_test.hpp>
# include <boost/test/test_tools.hpp>
# include <boost/thread.hpp>
//--------------------------------------------------------------------------------
# include "test_scene_1.h"
//--------------------------------------------------------------------------------
# include <mt_containers.h>
# include <condvar_queue.h>
# include <scmp_queue.h>
# include <scmp_ring_array.h>
# include <scsp_ring_array.h>
# include <scsp_ptr_ring_array.h>
//--------------------------------------------------------------------------------
namespace scene_1 {
//--------------------------------------------------------------------------------

template < typename ElemenT >
struct ContainerTraits< concur::RingArray< ElemenT > >
{
  typedef concur::RingArray< ElemenT > container_type;
  
  static SceneDesc::thread_func_type prod( ) { return &pexec_grab< container_type >; }
  static SceneDesc::thread_func_type cons( ) { return &cexec_grab< container_type >; }
  
  static void apply_to_scene( SceneDesc& ) { }
};

//--------------------------------------------------------------------------------

template < typename ElemenT >
struct ContainerTraits< concur::CondvarQueue< ElemenT > > {
  typedef concur::CondvarQueue< ElemenT > container_type;
  
  static SceneDesc::thread_func_type prod( ) { return &pexec< container_type >; }
  static SceneDesc::thread_func_type cons( ) { return &cexec_wait< container_type >; }
  
  static void apply_to_scene( SceneDesc& ) { }
};

//--------------------------------------------------------------------------------

template < typename ElemenT >
struct ContainerTraits< concur::CondvarRingArray< ElemenT > > {
  typedef concur::CondvarRingArray< ElemenT > container_type;
  
  static SceneDesc::thread_func_type prod( ) { return &pexec< container_type >; }
  static SceneDesc::thread_func_type cons( ) { return &cexec_wait< container_type >; }
  
  static void apply_to_scene( SceneDesc& ) { }
};

//--------------------------------------------------------------------------------

template < typename ElemenT >
struct ContainerTraits< concur::ScmpQueue< ElemenT > > {
  typedef concur::ScmpQueue< ElemenT > container_type;
  
  static SceneDesc::thread_func_type prod( ) { return &pexec< container_type >; }
  static SceneDesc::thread_func_type cons( ) { return &cexec< container_type >; }
  
  static void apply_to_scene( SceneDesc& desc ) {
    desc.prod_desc.thread_count   +=  desc.cons_desc.thread_count - 1;
    desc.cons_desc.thread_count   =   1;
  }
};

//--------------------------------------------------------------------------------

template < typename ElemenT, std::size_t Alignment >
struct ContainerTraits< concur::ScmpRingArray< ElemenT, Alignment > > {
  typedef concur::ScmpRingArray< ElemenT, Alignment > container_type;
  
  static SceneDesc::thread_func_type prod( ) { return &pexec< container_type >; }
  static SceneDesc::thread_func_type cons( ) { return &cexec< container_type >; }
  
  static void apply_to_scene( SceneDesc& desc ) {
    desc.prod_desc.thread_count   +=  desc.cons_desc.thread_count - 1;
    desc.cons_desc.thread_count   =   1;
  }
};

//--------------------------------------------------------------------------------

template < typename ElemenT, std::size_t Alignment >
struct ContainerTraits< concur::ScspRingArray< ElemenT, Alignment > > {
  typedef concur::ScspRingArray< ElemenT, Alignment > container_type;
  
  static SceneDesc::thread_func_type prod( ) { return &pexec< container_type >; }
  static SceneDesc::thread_func_type cons( ) { return &cexec< container_type >; }
  
  static void apply_to_scene( SceneDesc& desc ) {
    desc.prod_desc.thread_count = ( std::min )( 1u, desc.prod_desc.thread_count );
    desc.cons_desc.thread_count = ( std::min )( 1u, desc.cons_desc.thread_count );
  }
};

//--------------------------------------------------------------------------------

template < typename ElemenT, std::size_t Alignment >
struct ContainerTraits< concur::ScspPtrRingArray< ElemenT, Alignment > > {
  typedef concur::ScspPtrRingArray< ElemenT, Alignment > container_type;
  
  static SceneDesc::thread_func_type prod( ) { return &pexec_ptr< container_type >; }
  static SceneDesc::thread_func_type cons( ) { return &cexec_ptr< container_type >; }
  
  static void apply_to_scene( SceneDesc& desc ) {
    desc.prod_desc.thread_count = ( std::min )( 1u, desc.prod_desc.thread_count );
    desc.cons_desc.thread_count = ( std::min )( 1u, desc.cons_desc.thread_count );
  }
};

//--------------------------------------------------------------------------------
} // namespace scene_1
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE( SUITE_exchange_logic )
//--------------------------------------------------------------------------------

using namespace scene_1;

//--------------------------------------------------------------------------------

typedef concur::Queue< element_type, std::mutex, false >      mt_queue_std_type;
typedef concur::Queue< element_type, boost::mutex, false >    mt_queue_boost_type;
typedef concur::Queue< element_type, std::mutex, true >       mt_queue_std_limited_type;
typedef concur::Queue< element_type, boost::mutex, true >     mt_queue_boost_limited_type;
typedef concur::RingArray< element_type, std::mutex >         mt_ring_array_std_type;
typedef concur::RingArray< element_type, boost::mutex >       mt_ring_array_boost_type;
typedef concur::CondvarQueue< element_type, false >           condvar_queue_type;
typedef concur::CondvarQueue< element_type, true >            condvar_queue_limited_type;
typedef concur::CondvarRingArray< element_type >              condvar_ring_attay_type;
typedef concur::ScmpQueue< element_type >                     scmp_queue_type;
typedef concur::ScmpRingArray< element_type, 1 >              scmp_ring_array_type;
typedef concur::ScmpRingArray< element_type, 64 >             scmp_ring_array_a64_type;
typedef concur::ScmpRingArray< element_type, 128 >            scmp_ring_array_a128_type;
typedef concur::ScspRingArray< element_type, 1 >              scsp_ring_array_noalg_type;
typedef concur::ScspRingArray< element_type, 64 >             scsp_ring_array_a64_type;
typedef concur::ScspPtrRingArray< element_type, 1 >           scsp_ptr_ring_array_noalg_type;
typedef concur::ScspPtrRingArray< element_type, 64 >          scsp_ptr_ring_array_a64_type;

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_mt_queue_std )
{
  mt_queue_std_type container;
  container.init( get_config( ).container_capacity );
  run_exchange_test( container, "mt_queue_std" );
} // CASE_mt_queue_std
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_mt_queue_boost )
{
  mt_queue_boost_type container;
  container.init( get_config( ).container_capacity );
  run_exchange_test( container, "mt_queue_boost" );
} // CASE_mt_queue_boost
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_mt_queue_std_limited )
{
  mt_queue_std_limited_type container;
  container.init( get_config( ).container_capacity );
  run_exchange_test( container, "mt_queue_std_limited" );
} // CASE_mt_queue_std_limited
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_mt_queue_boost_limited )
{
  mt_queue_boost_limited_type container;
  container.init( get_config( ).container_capacity );
  run_exchange_test( container, "mt_queue_boost_limited" );
} // CASE_mt_queue_boost_limited
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_mt_ring_array_std )
{
  mt_ring_array_std_type container;
  container.init( get_config( ).container_capacity );
  run_exchange_test( container, "mt_ring_array_std" );
} // CASE_mt_ring_array_std
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_mt_ring_array_boost )
{
  mt_ring_array_boost_type container;
  container.init( get_config( ).container_capacity );
  run_exchange_test( container, "mt_ring_array_boost" );
} // CASE_mt_ring_array_boost
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_condvar_queue )
{
  condvar_queue_type container;
  run_exchange_test( container, "condvar_queue" );
} // CASE_condvar_queue
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_condvar_queue_limited )
{
  condvar_queue_limited_type container;
  container.init( get_config( ).container_capacity );
  run_exchange_test( container, "condvar_queue_limited" );
} // CASE_condvar_queue_limited
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_condvar_ring_array )
{
  condvar_ring_attay_type container;
  container.init( get_config( ).container_capacity );
  run_exchange_test( container, "condvar_ring_array" );
} // CASE_condvar_queue
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scmp_queue )
{
  scmp_queue_type container;
  run_exchange_test( container, "scmp_queue" );
} // CASE_scmp_queue
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scmp_ring_array )
{
  scmp_ring_array_type container;
  container.init( get_config( ).container_capacity );
  
  BOOST_TEST_MESSAGE( "ScmpRingArray: node alignment = " << scmp_ring_array_type::NODE_ALIGNMENT  );
  run_exchange_test( container, "scmp_ring_array" );
} // CASE_scmp_queue
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scmp_ring_array_a64 )
{
  scmp_ring_array_a64_type container;
  container.init( get_config( ).container_capacity );
  
  BOOST_TEST_MESSAGE( "ScmpRingArray: node alignment = " << scmp_ring_array_a64_type::NODE_ALIGNMENT  );
  run_exchange_test( container, "scmp_ring_array_a64" );
} // CASE_scmp_queue
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scmp_ring_array_a128 )
{
  scmp_ring_array_a128_type container;
  container.init( get_config( ).container_capacity );
  
  BOOST_TEST_MESSAGE( "ScmpRingArray: node alignment = " << scmp_ring_array_a128_type::NODE_ALIGNMENT  );
  run_exchange_test( container, "scmp_ring_array_a128" );
} // CASE_scmp_ring_array_a128
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scsp_ring_array_noalg )
{
  scsp_ring_array_noalg_type container;
  container.init( get_config( ).container_capacity );
  
  BOOST_TEST_MESSAGE( "ScspRingArray: no alignment"  );
  run_exchange_test( container, "scsp_ring_array_noalg" );
} // CASE_scsp_ring_array_noalg
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scsp_ring_array_a64 )
{
  scsp_ring_array_a64_type container;
  container.init( get_config( ).container_capacity );
  
  BOOST_TEST_MESSAGE( "ScspRingArray: 64 alignment"  );
  run_exchange_test( container, "scsp_ring_array_a64" );
} // CASE_scsp_ring_array_a64
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scsp_ptr_ring_array_noalg )
{
  scsp_ptr_ring_array_noalg_type container;
  container.init( get_config( ).container_capacity );
  
  BOOST_TEST_MESSAGE( "ScspPtrRingArray: no alignment"  );
  run_exchange_test( container, "scsp_ptr_ring_array_noalg" );
} // CASE_scsp_ptr_ring_array_noalg
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE( CASE_scsp_ptr_ring_array_a64 )
{
  scsp_ptr_ring_array_a64_type container;
  container.init( get_config( ).container_capacity );
  
  BOOST_TEST_MESSAGE( "ScspPtrRingArray: 64 alignment"  );
  run_exchange_test( container, "scsp_ptr_ring_array_a64" );
} // CASE_scsp_ptr_ring_array_a64
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END( ) // SUITE_exchange_logic
//--------------------------------------------------------------------------------

