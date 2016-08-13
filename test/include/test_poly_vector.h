#pragma once
#include "poly_vector.h"
#include <atomic>

struct Interface
{
    Interface() : my_id { last_id++ }{}
    Interface(const Interface& i) : my_id{ i.my_id } {}
    Interface(Interface&& i) : my_id{ i.my_id } {
        i.my_id = last_id++;
    }
    virtual void function() = 0;
    virtual Interface* clone( void* ) = 0;
    virtual Interface* move( void* dest ) = 0;
    virtual ~Interface() = default;
    bool operator<(const Interface& rhs)const noexcept
    {
        return my_id < rhs.my_id;
    }
    bool operator==(const Interface& rhs)const noexcept
    {
        return my_id == rhs.my_id;
    }
private:
    size_t my_id;
    static std::atomic<size_t> last_id;
};

class Impl1 : public Interface
{
public:
    Impl1( double dd = 0.0 ) : d{ dd } {}
    Impl1(const Impl1& i) : Interface(i) {}
    Impl1(Impl1&& o)noexcept : Interface(std::move(o)), d{ o.d } {}

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
    Impl2( Impl2&& o )noexcept : v{ std::move( o.v ) } {}
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

