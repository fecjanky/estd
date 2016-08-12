#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <iostream>
#include <vector>


#include "poly_vector.h"
#include "test_poly_vector.h"


void test_poly_vector() {
    int a = 0;
    {
        estd::poly_vector<Interface> v;
        estd::poly_vector<Interface,std::allocator<uint8_t>, estd::delegate_cloning_policy<Interface> > vdcp;
        v.push_back( Impl1{ 3.14 } );
        v.push_back( Impl1{ 6.28 } );
        v.push_back( Impl2{} );
        v.push_back( Impl2{} );
        v.push_back( Impl1{ 6.28 } );
        v.push_back( Impl2{} );
        
        vdcp.push_back( Impl1{ 3.14 } );
        vdcp.push_back( Impl2{} );

        auto vdcp2 = vdcp;
        for (auto i = v.begin(); i != v.end(); ++i) {
            i->function();
        }

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