# ifndef _CONFIG_H_
# define _CONFIG_H_
//--------------------------------------------------------------------------------
# include <cinttypes>
//--------------------------------------------------------------------------------

struct Config
{
  int64_t   iteration_count           = 1000000;
  int64_t   pool_capacity             = 1000;
  unsigned  releaser_thread_count     = 1;
  unsigned  consumer_thread_count     = 1;
  int       releaser_thread_affinity  = -1;
  int       consumer_thread_affinity  = -1;
  unsigned  repeat_count              = 1;
  bool      progress                  = false;
};

const Config& get_config( );

//--------------------------------------------------------------------------------
# endif // _CONFIG_H_

