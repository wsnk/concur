# define BOOST_TEST_ALTERNATIVE_INIT_API
# include "boost/test/included/unit_test.hpp"
//--------------------------------------------------------------------------------
# include "test_config.h"
//--------------------------------------------------------------------------------

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
     "\n  --list                  print test list"
     "\n  --op_count, -O          operation count"
     "\n  --container_size, -S    container capacity"
     "\n  --p_count, -P           producer thread count"
     "\n  --c_count, -C           consumer thread count"
     "\n  --p_affinity            producers CPU affinity"
     "\n  --c_affinity            consumers CPU affinity"
     "\n  --repeat, -R            repeat count"
     "\n";

//--------------------------------------------------------------------------------
    
bool init_unit_test( )
{
  using namespace boost::unit_test;
  master_test_suite_t& mts = framework::master_test_suite( );
  
  const unsigned  argc  = ( mts.argc >= 0 ) ? static_cast< unsigned >( mts.argc ) : 0;
  char** const    argv  = mts.argv;
  
  if( parse( "--help-h", argc, argv ) >= 0 ) {
    std::cout << HELP << std::endl;
    return false;
  }
  
  parse( cfg.operation_count,      "--op_count-O",  argc, argv );
  parse( cfg.container_capacity,   "--container_size-S",    argc, argv );
  parse( cfg.prod_thread_count,    "--p_count-P",   argc, argv );
  parse( cfg.cons_thread_count,    "--c_count-C",   argc, argv );
  parse( cfg.prod_thread_affinity, "--p_affinity",  argc, argv );
  parse( cfg.cons_thread_affinity, "--c_affinity",  argc, argv );
  parse( cfg.repeat_count,         "--repeat-R",    argc, argv );
  
  std::cout
      << "configuration:"
      << "\n  operation count:      " << cfg.operation_count
      << "\n  capacity:             " << cfg.container_capacity
      << "\n  prod. thread count:   " << cfg.prod_thread_count
      << "\n  cons. thread count:   " << cfg.cons_thread_count
      << "\n  prod. affinity begin: " << cfg.prod_thread_affinity
      << "\n  cons. affinity begin: " << cfg.cons_thread_affinity
      << "\n  repeat count:         " << cfg.repeat_count
      << "\n" << std::endl;
  
  return true;
}
