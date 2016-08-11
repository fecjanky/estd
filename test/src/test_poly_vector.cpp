#include "poly_vector.h"

#include <iostream>
#include <vector>

struct Interface {
    virtual void function() = 0;
    virtual Interface* clone(void*) = 0;
    virtual ~Interface() = default;
};

class Impl1 : public Interface {
public:
    Impl1(double dd = 0.0) : d{dd} {}
    void function() override { 
        d += 1.0;
    }
    virtual Interface* clone(void* dest) {
        return new (dest) Impl1(*this);
    }
private:
    double d;
};

class Impl2 : public Interface
{
public:
    Impl2() {}
    void function() override
    {
        v.push_back( Impl1{ 3.1 } );
    }
    virtual Interface* clone( void* dest )
    {
        return new (dest) Impl2( *this );
    }
private:
    estd::poly_vector<Interface> v;
};

struct alignas(64) ts
{
    int a;
};

void test_poly_vector() {
    int a = 0;
    {
        std::vector<ts> tv;
        tv.push_back( ts{ 2 } );
        tv.push_back( ts{ 4 } );
        for (auto& x : tv) {
            std::cout << x.a <<std::endl;
        }
        std::cout << reinterpret_cast<uint8_t*>( &tv[1]) - reinterpret_cast<uint8_t*>(&tv[0]) << std::endl;
        estd::poly_vector<Interface> v;
        estd::poly_vector<Interface,std::allocator<uint8_t>, estd::delegate_cloning_policy<Interface> > vdcp;
        v.push_back( Impl1{ 3.14 } );
        v.push_back( Impl1{ 6.28 } );
        v.push_back( Impl2{} );
        
        vdcp.push_back( Impl1{ 3.14 } );
        vdcp.push_back( Impl2{} );

        auto vdcp2 = vdcp;

        for (auto& i : v) {
            int a = 0;
            i.function();
        }
        auto v2 = v;
        v[0].function();
        v.pop_back();
        v.back().function();
        for (auto& i : v) {
            int a = 0;
        }
    }
    a = 1;

}