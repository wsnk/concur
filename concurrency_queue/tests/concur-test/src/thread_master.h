# ifndef _THREAD_MASTER_H_
# define _THREAD_MASTER_H_
//--------------------------------------------------------------------------------
# include <thread>
# include <functional>
# include <condition_variable>
# include <vector>
//--------------------------------------------------------------------------------
  
class ThreadMaster
{
public:
  struct Config
  {
    unsigned                            count     = 1;
    int                                 affinity  = -1;
    std::function< void( unsigned ) >   func;
  };
  
  ThreadMaster( const ThreadMaster& )               = delete;
  ThreadMaster& operator =( const ThreadMaster& )   = delete;
  
public:
  ThreadMaster( ) : started_( false ) { }
  ~ThreadMaster( ) { stop( ); }
  
  void initialize( const Config& cfg );
  void launch( );
  void stop( );
  
private:
  void run( unsigned num ) const ;
  
  Config                            cfg_;
  std::vector< std::thread >        threads_;
  mutable std::condition_variable   cv_;
  mutable std::mutex                mtx_;
  bool                              started_;
}; // struct ThreadHolder

//--------------------------------------------------------------------------------
# endif // _THREAD_MASTER_H_
