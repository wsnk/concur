# ifndef _TEST_CONFIG_H_
# define _TEST_CONFIG_H_
//--------------------------------------------------------------------------------
#include <cstdint>
//--------------------------------------------------------------------------------

struct Config
{
  unsigned  cons_thread_count       = 1;
  unsigned  prod_thread_count       = 1;
  unsigned  container_capacity      = 10000;
  int64_t   operation_count         = 1000000;
  int       prod_thread_affinity    = -1;
  int       cons_thread_affinity    = -1;
  unsigned  repeat_count            = 1;
};

const Config& get_config( );

//--------------------------------------------------------------------------------
# endif // _TEST_CONFIG_H_

