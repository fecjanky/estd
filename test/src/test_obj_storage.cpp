#include <exception>
#include <tuple>

#include "memory.h"
#include "test_obj_storage.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#ifdef _MSC_VER
__declspec(dllexport)
#endif  // _MSC_VER
int PullInTestObjStorageLibrary() { return 0; }

namespace ObjStorageTest {

bool inline_allocation_happened(estd::sso_storage& o) {
    o.get_checked();
    auto obj_addr = reinterpret_cast<uintptr_t>(&o);
    auto alloc_addr = reinterpret_cast<uintptr_t>(o.get());
    return o.get() && (alloc_addr - obj_addr) < sizeof(o);
}

TEST(ObjStorageTest, size_is_0_after_default_construction) {
    estd::sso_storage o{};
    EXPECT_EQ(0, o.size());
}

TEST(ObjStorageTest, pointer_obtained_with_get_is_nullptr_after_default_construction) {
    estd::sso_storage o{};
    EXPECT_EQ(nullptr, o.get());
}

TEST(ObjStorageTest, get_checked_throws_exception_when_default_constructed) {
     EXPECT_THROW(estd::sso_storage{}.get_checked(), std::exception);
}

TEST(ObjStorageTest, max_size_is_integral_multiple_of_pointer_size) {
    EXPECT_EQ(0, estd::sso_storage::max_size() % sizeof(void*));
}

TEST(ObjStorageTest, max_size_is_greater_or_equal_than_pointer_size) {
    EXPECT_GE(estd::sso_storage::max_size(), sizeof(void*));
}


TEST(ObjStorageTest, size_is_greater_or_equal_to_allocation_size_after_allocation) {
    estd::sso_storage o1{}, o2{};
    o1.allocate(10);
    o2.allocate(100);
    EXPECT_GE(o1.size(), 10);
    EXPECT_GE(o2.size(), 100);
}

TEST(ObjStorageTest, inline_allocation_happens_when_allocation_size_is_less_than_or_equals_to_max_size) {
    estd::sso_storage o1{};
    o1.allocate(estd::sso_storage::max_size());
    EXPECT_TRUE(inline_allocation_happened(o1));
}

TEST(ObjStorageTest, heap_allocation_happens_when_allocation_size_is_greater_than_max_size) {
    estd::sso_storage o1{};
    o1.allocate(estd::sso_storage::max_size()+1);
    EXPECT_FALSE(inline_allocation_happened(o1));
}

TEST(ObjStorageTest, zero_size_allocation_results_in_size_1) {
    estd::sso_storage o1{};
    o1.allocate(0);
    EXPECT_EQ(1,o1.size());
}

TEST(ObjStorageTest, same_size_storages_compares_equal) {
    using ::testing::Eq;
    estd::sso_storage obj_0_1{100}, obj_0_2{100};
    ASSERT_TRUE(obj_0_1.get_allocator() == obj_0_2.get_allocator());
    EXPECT_TRUE(obj_0_1 == obj_0_2);
}



INSTANTIATE_TEST_CASE_P(
    ObjStorage,
    AllocationTest,
    ::testing::Values(
        0,
        1,
        estd::sso_storage::max_size(),
        estd::sso_storage::max_size()+1,
        estd::sso_storage::max_size()*10)
    );


// Param 0 - storage size in ptr
// Param 1 - alignment size
// Param 2 - PropOnCCopy 
// Param 3 - PropOnCMove 
// Param 4 - PropOnCSwap 
// Param 5 - initial alloc size in bytes


using AllocationTestInlineTypes = ::testing::Types<
    std::tuple<Size<4>, Size<8>, std::true_type, std::true_type, std::true_type, Size<0>>,
    std::tuple<Size<4>, Size<8>, std::true_type, std::true_type, std::true_type, Size<1>>,
    std::tuple<Size<4>, Size<8>, std::true_type, std::true_type, std::true_type, Size<4 * sizeof(void*)>>
>;

using AllocationTestAllocatedTypes = ::testing::Types<
    std::tuple<Size<4>, Size<8>, std::true_type, std::true_type, std::true_type, Size<4 * sizeof(void*)+1>>,
    std::tuple<Size<4>, Size<8>, std::true_type, std::true_type, std::true_type, Size<8 * sizeof(void*)>>
>;

using AllocationTestTypes = ::testing::Types<
    std::tuple<Size<4>, Size<8>, std::true_type, std::true_type, std::true_type, Size<8 * sizeof(void*)>>,
    std::tuple<Size<4>, Size<8>, std::false_type, std::true_type, std::true_type, Size<8 * sizeof(void*)>>,
    std::tuple<Size<4>, Size<8>, std::true_type, std::false_type, std::true_type, Size<8 * sizeof(void*)>>,
    std::tuple<Size<4>, Size<8>, std::true_type, std::true_type, std::false_type, Size<8 * sizeof(void*)>>,
    std::tuple<Size<8>, Size<8>, std::true_type, std::true_type, std::true_type, Size<8 * sizeof(void*)>>,
    std::tuple<Size<8>, Size<8>, std::false_type, std::true_type, std::true_type, Size<8 * sizeof(void*)>>,
    std::tuple<Size<8>, Size<8>, std::true_type, std::false_type, std::true_type, Size<8 * sizeof(void*)>>,
    std::tuple<Size<8>, Size<8>, std::true_type, std::true_type, std::false_type, Size<8 * sizeof(void*)>>
>;


INSTANTIATE_TYPED_TEST_CASE_P(ObjStorage, Test, AllocationTestTypes);

INSTANTIATE_TYPED_TEST_CASE_P(ObjStorage, TestInlineAllocation, AllocationTestInlineTypes);

INSTANTIATE_TYPED_TEST_CASE_P(ObjStorage, TestAllocation, AllocationTestAllocatedTypes);


}
