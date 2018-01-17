# ifndef _CONTAINER_GRAB_H_
# define _CONTAINER_GRAB_H_
//--------------------------------------------------------------------------------
# include <cassert>
//--------------------------------------------------------------------------------
namespace concur {
//--------------------------------------------------------------------------------

// Grab allows invoke 'push', 'pop' and 'size' container methods multiple times
// under the same lock.
//
// Type 'ContainerT' must provide:
//    void          unlock( );
//    void          lock( );
//    bool          put_element( T&& src );
//    bool          get_element( T&  dst );
//    std::size_t   get_size( );

//--------------------------------------------------------------------------------

template < typename ContainerT >
class ContainerGrab
{
public:
  typedef ContainerT container_type;
  
  ContainerGrab( const ContainerGrab& )               = delete;
  ContainerGrab& operator =( const ContainerGrab& )   = delete;
  
public:
  explicit ContainerGrab( container_type& container ) : cont_( &container )
  { cont_->lock( ); }
  
  ContainerGrab( ContainerGrab&& x ) : cont_( x.cont_ )
  { x.cont_ = nullptr; }
  
  ~ContainerGrab( ) { if( cont_ ) cont_->unlock( ); }
  
  inline operator bool( ) const { return cont_ != nullptr; }
  
  template < typename T >
  inline bool push( T&& src ) {
    assert( *this );
    return cont_->put_element( std::forward< T >( src ) );
  }
  
  template < typename T >
  inline bool pop( T& dst ) {
    assert( *this );
    return cont_->get_element( dst );
  }
  
  inline std::size_t size( ) const {
    assert( *this );
    return cont_->get_size( );
  }
  
private:
  container_type* cont_;
}; // class ContainerGrab

//--------------------------------------------------------------------------------

template < typename ContainerT >
inline ContainerGrab< ContainerT > grab( ContainerT& container ) {
  return ContainerGrab< ContainerT >( container );
}

//--------------------------------------------------------------------------------
} // namespace concur
//--------------------------------------------------------------------------------
# endif // _CONTAINER_GRAB_H_
