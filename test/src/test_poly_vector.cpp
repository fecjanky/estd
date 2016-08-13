#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <iostream>
#include <vector>


#include "poly_vector.h"
#include "test_poly_vector.h"

#ifdef _MSC_VER
__declspec(dllexport)
#endif  // _MSC_VER
int PullInTestPolyVectorLibrary() { return 0; }

std::atomic<size_t> Interface::last_id = 0;

TEST(poly_vector_basic_tests, default_constructed_poly_vec_is_empty)
{
    estd::poly_vector<Interface> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(0, v.size());
    EXPECT_EQ(0, v.sizes().first);
    EXPECT_EQ(0, v.sizes().second);
    EXPECT_EQ(0, v.capacity());
    EXPECT_EQ(0, v.capacities().first);
    EXPECT_EQ(0, v.capacities().second);
}


TEST(poly_vector_basic_tests, reserve_increases_capacity)
{
    estd::poly_vector<Interface> v;
    constexpr size_t n = 16;
    constexpr size_t avg_s = 64;
    v.reserve(n, 64);
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(0, v.size());
    EXPECT_EQ(0, v.sizes().first);
    EXPECT_EQ(0, v.sizes().second);
    EXPECT_EQ(n, v.capacity());
    EXPECT_EQ(n, v.capacities().first);
    EXPECT_LE(n*avg_s, v.capacities().second);
}

void test_poly_vector() {
    int a = 0;
    {
        estd::poly_vector<Interface> v,v2;
        estd::poly_vector<Interface,std::allocator<Interface>, estd::delegate_cloning_policy<Interface> > vdcp;
        v.push_back( Impl1{ 3.14 } );
        v.push_back( Impl1{ 6.28 } );
        v.push_back( Impl2{} );
        v.push_back( Impl2{} );
        v.push_back( Impl1{ 6.28 } );
        v.push_back( Impl2{} );
        
        v2 = v;

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
        auto v3 = v;
        v[0].function();
        v.pop_back();
        v.back().function();
        for (auto& i : v) {
            int a = 0;
        }
    }
    a = 1;

}