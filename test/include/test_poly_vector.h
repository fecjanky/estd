#pragma once
#include "poly_vector.h"

struct Interface
{
    virtual void function() = 0;
    virtual Interface* clone( void* ) = 0;
    virtual Interface* move( void* dest ) = 0;
    virtual ~Interface() = default;
};

class Impl1 : public Interface
{
public:
    Impl1( double dd = 0.0 ) : d{ dd } {}
    void function() override
    {
        d += 1.0;
    }
    virtual Interface* clone( void* dest )
    {
        return new (dest) Impl1( *this );
    }
    virtual Interface* move( void* dest )
    {
        return new (dest) Impl1( std::move( *this ) );
    }
private:
    double d;
};

class Impl2 : public Interface
{
public:
    Impl2() {}
    Impl2( const Impl2& ) = default;
    Impl2( Impl2&& o )noexcept : v{ std::move( v ) } {}
    void function() override
    {
        v.push_back( Impl1{ 3.1 } );
    }
    virtual Interface* clone( void* dest )
    {
        return new (dest) Impl2( *this );
    }
    virtual Interface* move( void* dest )
    {
        return new (dest) Impl2( std::move( *this ) );
    }

private:
    estd::poly_vector<Interface> v;
};

