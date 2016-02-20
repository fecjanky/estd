#ifndef TEST_OBJ_STORAGE_H_
#define TEST_OBJ_STORAGE_H_

#include "gtest/gtest.h"
#include "memory.h"

namespace ObjStorageTest {

class AllocationTest : public ::testing::TestWithParam<size_t> {
public:
    AllocationTest() : s(GetParam()) {}
protected:
    estd::obj_storage s;
};


TEST_P(AllocationTest, ConstructionAndSize) {
    if (GetParam() == 0) {
        EXPECT_EQ(s.size(), 1);
    }
    else if (GetParam() <= s.max_size())
    {
        EXPECT_EQ(s.size(), GetParam());
    }
    else {
        EXPECT_GE(s.size(), GetParam());
    }
}

TEST_P(AllocationTest, ConstructionAndPtr) {

    ASSERT_NO_THROW(s.get_checked());
    EXPECT_NE(nullptr, s.get_checked());

    auto p = reinterpret_cast<uintptr_t>(s.get());
    auto ps = reinterpret_cast<uintptr_t>(&s);

    auto diff = p - ps;

    EXPECT_EQ(
        0, 
        reinterpret_cast<uintptr_t>(s.get()) % estd::obj_storage::alignment);

    if (GetParam() <= s.max_size())
    {
        EXPECT_LT(diff,sizeof(s));
    }
    else {
        EXPECT_GE(diff,sizeof(s));
    }
}

TEST_P(AllocationTest, CopyConstruction) {

    estd::obj_storage s2 = s;
    EXPECT_EQ(s2.get_allocator(), s.get_allocator());
    EXPECT_EQ(s2.size(), s.size());
    ASSERT_EQ(s2.max_size(), s.max_size());
    EXPECT_NE(s2.get(), s.get());
    
}

TEST_P(AllocationTest, MoveConstruction) {

    auto old_storage = s.get();
    estd::obj_storage s2 = std::move(s);
    if (s2.size() <= s2.max_size()) {
        EXPECT_EQ(s2.size(), s.size());
        EXPECT_NE(s2.get(), s.get());
    } else {
        EXPECT_EQ(old_storage, s2.get());
    }
}

TEST_P(AllocationTest, CopyAssigmentInline) {

    estd::obj_storage s2(1);
    s2 = s;
    EXPECT_EQ(s2.get_allocator(), s.get_allocator());
    EXPECT_EQ(s2.size(), s.size());
    EXPECT_NE(s2.get(), s.get());
}

TEST_P(AllocationTest, CopyAssigmentAllocated) {

    estd::obj_storage s2(s.max_size()+1);
    s2 = s;
    EXPECT_EQ(s2.get_allocator(), s.get_allocator());
    EXPECT_EQ(s2.size(), s.size());
    EXPECT_NE(s2.get(), s.get());
}

TEST_P(AllocationTest, MoveAssignment) {

    estd::obj_storage s_i(1);
    estd::obj_storage s_a(s.max_size()+1);
    auto old_storage = s.get();
    auto size = s.size();

    if (s.size() <= s.max_size()) {
        s_i = std::move(s);
        EXPECT_EQ(s_i.size(), s.size());
        EXPECT_NE(s_i.get(), s.get());
        s_a = std::move(s);
        EXPECT_EQ(s_a.size(), s.size());
        EXPECT_NE(s_a.get(), s.get());
    }
    else {
        s_i = std::move(s);
        EXPECT_EQ(size, s_i.size());
        EXPECT_EQ(old_storage, s_i.get());
        EXPECT_EQ(nullptr, s.get());
        EXPECT_EQ(0, s.size());
        s_a = std::move(s_i);
        EXPECT_EQ(size, s_a.size());
        EXPECT_EQ(old_storage, s_a.get());
        EXPECT_EQ(nullptr, s_i.get());
        EXPECT_EQ(0, s_i.size());

    }
}

// TODO: mock allocator and add test cases

}
#endif  // TEST_OBJ_STORAGE_H_
