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
    v.reserve(n, avg_s);
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(0, v.size());
    EXPECT_EQ(0, v.sizes().first);
    EXPECT_EQ(0, v.sizes().second);
    EXPECT_LE(n, v.capacity());
    EXPECT_LE(n, v.capacities().first);
    EXPECT_LE(n*avg_s, v.capacities().second);
}

TEST(poly_vector_basic_tests, reserve_does_not_increase_capacity_when_size_is_less_than_current_capacity)
{
    estd::poly_vector<Interface> v;
    constexpr size_t n = 16;
    constexpr size_t avg_s = 8;
    v.reserve(n, avg_s);
    const auto capacities = v.capacities();
    v.reserve(n / 2, avg_s);
    EXPECT_EQ(capacities, v.capacities());
}

TEST(poly_vector_basic_tests, descendants_of_interface_can_be_pushed_back_into_the_vector)
{
    estd::poly_vector<Interface> v;
    v.push_back(Impl1(3.14));
    v.push_back(Impl2());
    EXPECT_EQ(2, v.size());
}


class poly_vector_modifier_test : public ::testing::Test {
public:
    poly_vector_modifier_test() {
        v.push_back(obj1);
        v.push_back(obj2);
    }
protected:
    Impl1 obj1;
    Impl2 obj2;
    estd::poly_vector<Interface> v;
};

TEST_F(poly_vector_modifier_test, back_accesses_the_last_element)
{
    Interface& obj = this->obj2;
    EXPECT_EQ(obj ,v.back());
}

TEST_F(poly_vector_modifier_test, front_accesses_the_first_element)
{
    Interface& obj = this->obj1;
    EXPECT_EQ(obj, v.front());
}

TEST_F(poly_vector_modifier_test, pop_back_removes_the_last_element)
{
    Interface& obj1 = this->obj1;
    Interface& obj2 = this->obj2;
    EXPECT_EQ(obj2, v.back());
    v.pop_back();
    EXPECT_EQ(obj1, v.back());
}

TEST_F(poly_vector_modifier_test, pop_back_keeps_capacity_but_adjustes_size)
{
    auto cap = v.capacities();
    v.pop_back();
    v.pop_back();
    EXPECT_EQ(cap, v.capacities());
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(std::make_pair(size_t(0),size_t(0)),v.sizes());
}

TEST_F(poly_vector_modifier_test, clear_destroys_all_elements_but_keeps_capacity)
{
    v.push_back(Impl2{});
    v.push_back(Impl1{6.28});
    auto cap = v.capacities();
    v.clear();
    EXPECT_EQ(cap, v.capacities());
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(std::make_pair(size_t(0), size_t(0)), v.sizes());
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