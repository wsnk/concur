# ifndef _CONCUR_MEM_UTILS_H_
# define _CONCUR_MEM_UTILS_H_
//--------------------------------------------------------------------------------
# include <cstdint>
# include <cstdlib>
# include <cassert>
# include <stdexcept>
//--------------------------------------------------------------------------------
# ifdef _MSC_VER 
#   define ALIGNAS( V )   __declspec( align( 64 ) ) // unable to use constant in MSVC
#   define ALIGNOF( T )   __alignof( T )
# else
#   define ALIGNAS( V )   alignas( V )
#   define ALIGNOF( T )   alignof( T )
# endif
//--------------------------------------------------------------------------------
# ifdef _WIN32
#   define aligned_malloc( A, S )   _aligned_malloc( S, A )
#   define aligned_free( PTR )      _aligned_free( PTR )
# else
#   include <malloc.h>
#   define aligned_malloc( A, S )   memalign( A, S )
#   define aligned_free( PTR )      std::free( PTR )
# endif
//--------------------------------------------------------------------------------
namespace concur { namespace utils {
//--------------------------------------------------------------------------------

template < typename ElementType, typename SizeType >
class aligned_ring
{
public:
  typedef ElementType   element_type;
  typedef SizeType      size_type;
  
  aligned_ring( const aligned_ring& )               = delete;
  aligned_ring& operator =( const aligned_ring& )   = delete;
  
public:
  aligned_ring( ) = default;
  
  ~aligned_ring( )
  {
    for( size_type i( 0 ); i < arr_size_; ++i )
      arr_[ i ].~element_type( );
    aligned_free( arr_ );
  }
  
  void init( size_type size )
  {
    assert( ( arr_ == nullptr ) && "ring already initialized" );
    
    arr_ = static_cast< element_type* >(
             aligned_malloc( ALIGNOF( element_type ), sizeof( element_type ) * size )
             );
    
    if( !arr_ )
      throw std::bad_alloc( );
    
    arr_size_ = size;
    for( size_type i( 0 ); i < arr_size_; ++i )
      new( arr_ + i ) element_type;
  }
  
  inline element_type& at( size_type i ) { return arr_[ i % arr_size_ ]; }
  
  inline size_type size( ) const { return arr_size_; }

private:
  element_type*   arr_        = nullptr;
  size_type       arr_size_   = 0;
}; // class aligned_ring

//--------------------------------------------------------------------------------
} } // namespace concur::utils
//--------------------------------------------------------------------------------
# endif // _CONCUR_MEM_UTILS_H_
