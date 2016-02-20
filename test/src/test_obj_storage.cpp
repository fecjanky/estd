#include <exception>

#include "gtest/gtest.h"
#include "memory.h"
#include "test_obj_storage.h"

__declspec(dllexport) int PullInTestObjStorageLibrary() { return 0; }

namespace ObjStorageTest {

TEST(ObjStorageTest, DefaultConstruction) {
    estd::obj_storage o;
    EXPECT_EQ(0, o.size());
    EXPECT_EQ(nullptr, o.get());
    EXPECT_THROW(o.get_checked(), std::exception);
}


INSTANTIATE_TEST_CASE_P(
    ObjStorage,
    AllocationTest,
    ::testing::Values(
        0,
        1,
        estd::obj_storage::max_size(),
        estd::obj_storage::max_size()+1)
    );

}