#include <exception>
#include <tuple>

#include "gtest/gtest.h"
#include "memory.h"
#include "test_obj_storage.h"
#include "mock_allocator.h"

#ifdef MSVC
__declspec(dllexport)
#endif //MSVC
int PullInTestObjStorageLibrary() { return 0; }

namespace ObjStorageTest {

TEST(ObjStorageBasicTest, DefaultConstruction) {
    obj_storage_test o;
    ASSERT_EQ(def_storage_size*sizeof(void*), o.max_size());
    EXPECT_EQ(true, o.get_allocator().def_constructed);
    EXPECT_EQ(0U, o.size());
    EXPECT_EQ(nullptr, o.get());
    EXPECT_THROW(o.get_checked(), std::exception);
}


TEST(ObjStorageBasicTest, AllocatorCopyConstruction) {
    Mock::AllocatorMock<uint8_t> a;
    obj_storage_test o(std::allocator_arg_t{}, a);
    EXPECT_EQ(true, o.get_allocator().copy_constructed);
}


TEST(ObjStorageBasicTest, AllocatorMoveConstruction) {
    Mock::AllocatorMock<uint8_t> a;
    obj_storage_test o(std::allocator_arg_t{}, std::move(a));
    EXPECT_EQ(true, o.get_allocator().move_constructed);
}


INSTANTIATE_TEST_CASE_P(
    ObjStorage,
    AllocationTest,
    ::testing::Values(
        0,
        1,
        estd::obj_storage::max_size(),
        estd::obj_storage::max_size()+1,
        estd::obj_storage::max_size()*10)
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
