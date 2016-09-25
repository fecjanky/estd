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

std::atomic<size_t> Interface::last_id {0};

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

TEST(poly_vector_basic_tests, is_not_copyable_with_no_cloning_policy)
{
    estd::poly_vector<Interface, std::allocator<Interface>, estd::no_cloning_policy<Interface>> v{};
    v.reserve(2, 64);
    v.push_back(Impl1(3.14));
    v.push_back(Impl2());
    EXPECT_THROW(v.push_back(Impl2()), std::exception);
}

template<class CloningPolicy>
class poly_vector_modifiers_test : public ::testing::Test {
public:
    using vector = estd::poly_vector<Interface, std::allocator<Interface>, CloningPolicy>;

    poly_vector_modifiers_test() {
        v.push_back(obj1);
        v.push_back(obj2);
    }
protected:
    Impl1 obj1;
    Impl2T<CloningPolicy> obj2;
    vector v;
};

TYPED_TEST_CASE_P(poly_vector_modifiers_test);

using CloneTypes = ::testing::Types<
    estd::virtual_cloning_policy<Interface>,
    estd::delegate_cloning_policy<Interface>
>;

TYPED_TEST_P(poly_vector_modifiers_test, back_accesses_the_last_element)
{
    Interface& obj = this->obj2;
    EXPECT_EQ(obj , this->v.back());
}

TYPED_TEST_P(poly_vector_modifiers_test, front_accesses_the_first_element)
{
    Interface& obj = this->obj1;
    EXPECT_EQ(obj, this->v.front());
}

TYPED_TEST_P(poly_vector_modifiers_test, pop_back_removes_the_last_element)
{
    Interface& obj1 = this->obj1;
    Interface& obj2 = this->obj2;
    EXPECT_EQ(obj2, this->v.back());
    
    this->v.pop_back();
    
    EXPECT_EQ(obj1, this->v.back());
}

TYPED_TEST_P(poly_vector_modifiers_test, pop_back_keeps_capacity_but_adjustes_size)
{
    auto cap = this->v.capacities();
    
    this->v.pop_back();
    this->v.pop_back();
    
    EXPECT_EQ(cap, this->v.capacities());
    EXPECT_TRUE(this->v.empty());
    EXPECT_EQ(std::make_pair(size_t(0),size_t(0)), this->v.sizes());
}

TYPED_TEST_P(poly_vector_modifiers_test, clear_destroys_all_elements_but_keeps_capacity)
{
    using Impl2 = Impl2T<TypeParam>;
    this->v.push_back(Impl2{});
    this->v.push_back(Impl1{6.28});
    auto cap = this->v.capacities();
    
    this->v.clear();
    
    EXPECT_EQ(cap, this->v.capacities());
    EXPECT_TRUE(this->v.empty());
    EXPECT_EQ(std::make_pair(size_t(0), size_t(0)), this->v.sizes());
}

TYPED_TEST_P(poly_vector_modifiers_test, at_operator_retrieves_nth_elem)
{
    EXPECT_EQ(static_cast<Interface&>(this->obj1), this->v[0]);
    EXPECT_EQ(static_cast<Interface&>(this->obj2), this->v[1]);
}

TYPED_TEST_P(poly_vector_modifiers_test, at_operator_throws_out_of_range_error_on_overindexing)
{
    EXPECT_NO_THROW(this->v.at(1));
    EXPECT_THROW(this->v.at(2), std::out_of_range);
}

TYPED_TEST_P(poly_vector_modifiers_test, swap_swaps_the_contents_of_two_vectors)
{
    using Impl2 = Impl2T<TypeParam>;
    using vector = typename poly_vector_modifiers_test<TypeParam>::vector;
    vector v2;
    Impl1 obj1{ 6.28 };
    Impl2 obj2{};
    v2.push_back(obj1);
    v2.push_back(obj2);
    
    this->v.swap(v2);
    
    EXPECT_EQ(static_cast<Interface&>(obj1),this->v[0]);
    EXPECT_EQ(static_cast<Interface&>(obj2), this->v[1]);
    EXPECT_EQ(static_cast<Interface&>(this->obj1), v2[0]);
    EXPECT_EQ(static_cast<Interface&>(this->obj2), v2[1]);

}

TYPED_TEST_P(poly_vector_modifiers_test, copy_assignment_copies_contained_elems)
{
    using vector = typename poly_vector_modifiers_test<TypeParam>::vector;
    vector v2;

    v2 = this->v;

    EXPECT_EQ(static_cast<Interface&>(this->obj1), v2[0]);
    EXPECT_EQ(static_cast<Interface&>(this->obj2), v2[1]);
}

TYPED_TEST_P(poly_vector_modifiers_test, move_assignment_moves_contained_elems)
{
    using vector = typename poly_vector_modifiers_test<TypeParam>::vector;
    vector v2;
    auto size = v2.sizes();
    auto capacities = v2.capacities();

    v2 = std::move(this->v);

    EXPECT_EQ(static_cast<Interface&>(this->obj1), v2[0]);
    EXPECT_EQ(static_cast<Interface&>(this->obj2), v2[1]);
    EXPECT_EQ(size, this->v.sizes());
    EXPECT_EQ(capacities, this->v.capacities());

}

TYPED_TEST_P(poly_vector_modifiers_test, strog_guarantee_when_there_is_an_exception_during_push_back_w_reallocation)
{

    this->v[1].set_throw_on_copy_construction(true);
    while (this->v.capacity() - this->v.size() > 0) {
        this->v.push_back(Impl1{});
    }
    if (!std::is_same<TypeParam, estd::delegate_cloning_policy<Interface>>{}) {
        std::vector<uint8_t> ref_storage(static_cast<uint8_t*>(this->v.data().first), static_cast<uint8_t*>(this->v.data().second));
        auto data = this->v.data();
        auto size = this->v.sizes();
        auto caps = this->v.capacities();

        EXPECT_THROW(this->v.push_back(Impl2{}), std::exception);

        EXPECT_EQ(data, this->v.data());
        EXPECT_EQ(size, this->v.sizes());
        EXPECT_EQ(caps, this->v.capacities());
        EXPECT_TRUE(0 == memcmp(ref_storage.data(), this->v.data().first, ref_storage.size()));
    }
    else {
        EXPECT_NO_THROW(this->v.push_back(Impl2{}));
    }
}

REGISTER_TYPED_TEST_CASE_P(poly_vector_modifiers_test,
    back_accesses_the_last_element, 
    front_accesses_the_first_element,
    pop_back_removes_the_last_element,
    pop_back_keeps_capacity_but_adjustes_size,
    clear_destroys_all_elements_but_keeps_capacity,
    at_operator_retrieves_nth_elem,
    at_operator_throws_out_of_range_error_on_overindexing,
    swap_swaps_the_contents_of_two_vectors,
    copy_assignment_copies_contained_elems,
    move_assignment_moves_contained_elems,
    strog_guarantee_when_there_is_an_exception_during_push_back_w_reallocation
);

INSTANTIATE_TYPED_TEST_CASE_P(poly_vector, poly_vector_modifiers_test, CloneTypes);

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