# define BOOST_TEST_ALTERNATIVE_INIT_API
# include "boost/test/included/unit_test.hpp"
//--------------------------------------------------------------------------------
# include "config.h"
//--------------------------------------------------------------------------------
# include <thread>
//--------------------------------------------------------------------------------
namespace {
//--------------------------------------------------------------------------------

static Config cfg;

//--------------------------------------------------------------------------------

int parse( const char* names, unsigned argc, char** argv )
{
  for( unsigned i( 0 ); i < argc; ++i )
    if( std::strstr( names, argv[ i ] ) )
      return i;
  return -1;
}

bool parse( const char*& dst, const char* names, unsigned argc, char** argv )
{
  for( unsigned i( 0 ); i < argc; ++i )
    if( std::strstr( names, argv[ i ] ) ) {
      if( i == ( argc - 1 ) )
        return false; // no value
      dst = argv[ i + 1 ];
      return true;
    }
  return false;
}

bool parse_double( double& dst, const char* names, unsigned argc, char** argv )
{
  const char* val = nullptr;
  if( parse( val, names, argc, argv ) ) {
    assert( val );
    dst = std::atof( val );
    return true;
  }
  return false;
}

bool parse( unsigned& dst, const char* names, unsigned argc, char** argv )
{
  const char* str_val = nullptr;
  if( parse( str_val, names, argc, argv ) ) {
    long long int val = std::atoll( str_val );
    if( val >= 0 && val <= UINT_MAX ) {
      dst = static_cast< unsigned >( val );
      return true;
    }
  }
  return false;
}

bool parse( int& dst, const char* names, unsigned argc, char** argv )
{
  const char* str_val = nullptr;
  if( parse( str_val, names, argc, argv ) ) {
    long long int val = std::atoll( str_val );
    if( val >= 0 && val <= INT_MAX ) {
      dst = static_cast< int >( val );
      return true;
    }
  }
  return false;
}

bool parse( int64_t& dst, const char* names, unsigned argc, char** argv )
{
  const char* str_val = nullptr;
  if( parse( str_val, names, argc, argv ) ) {
    long long int val = std::atoll( str_val );
    if( ( val >= INT64_MIN ) && ( val <= INT64_MAX ) ) {
      dst = static_cast< int >( val );
      return true;
    }
  }
  return false;
}

//--------------------------------------------------------------------------------
} // anonymous namespace
//--------------------------------------------------------------------------------

const Config& get_config( ) { return cfg; }

//--------------------------------------------------------------------------------

const char* const HELP =
    "parameters:"
     "\n  --op_count, -O          operation count"
     "\n  --container_size, -S    container capacity"
     "\n  --ccount, -C            consumer thread count"
     "\n  --rcount, -R            releaser thread count"
     "\n  --caff                  consumer CPU affinity"
     "\n  --raff                  releaser CPU affinity"
     "\n";

//--------------------------------------------------------------------------------
    
bool init_unit_test( )
{
  using namespace boost::unit_test;
  master_test_suite_t& mts = framework::master_test_suite( );
  
  const unsigned  argc  = ( mts.argc >= 0 ) ? static_cast< unsigned >( mts.argc ) : 0;
  char** const    argv  = mts.argv;
  
  const unsigned CPU_COUNT = std::thread::hardware_concurrency( );
  
  // default threads count
  cfg.releaser_thread_count = ( CPU_COUNT / 2 ) ? ( CPU_COUNT / 2 ) : 1;
  cfg.consumer_thread_count = cfg.releaser_thread_count;
  
  if( parse( "--help-h", argc, argv ) >= 0 ) {
    std::cout << HELP << std::endl;
    return false;
  }
  
  parse( cfg.iteration_count,           "--iter_count-O",   argc, argv );
  parse( cfg.pool_capacity,             "--pool_size-S",    argc, argv );
  parse( cfg.releaser_thread_count,     "--rcount-R",       argc, argv );
  parse( cfg.consumer_thread_count,     "--ccount-C",       argc, argv );
  parse( cfg.releaser_thread_affinity,  "--raff",           argc, argv );
  parse( cfg.consumer_thread_affinity,  "--caff",           argc, argv );
  parse( cfg.repeat_count,              "--repeat",         argc, argv );
  
  cfg.progress = parse( "--progress", argc, argv ) != -1;
  
  std::cout
      << "CPU count: " << CPU_COUNT << "\n"
      << "configuration:"
      << "\n  iteration count:      " << cfg.iteration_count
      << "\n  pool capacity:        " << cfg.pool_capacity
      << "\n  cons. thread count:   " << cfg.consumer_thread_count
      << "\n  rel. thread count:    " << cfg.releaser_thread_count
      << "\n  cons. affinity begin: " << cfg.consumer_thread_affinity
      << "\n  rel. affinity begin:  " << cfg.releaser_thread_affinity
      << "\n  repeat count:         " << cfg.repeat_count
      << "\n  progress:             " << cfg.progress
      << "\n" << std::endl;
  
  return true;
}

//--------------------------------------------------------------------------------
