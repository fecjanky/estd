#ifndef TEST_OBJ_STORAGE_H_
#define TEST_OBJ_STORAGE_H_

#include <tuple>
#include <cmath>

#include "gtest/gtest.h"
#include "memory.h"
#include "mock_allocator.h"

namespace ObjStorageTest {

constexpr size_t def_storage_size = 4;

template<
    size_t storage_size = def_storage_size, // in pointer size
    size_t alignment = alignof(std::max_align_t),
    class PropOnCCopy = std::true_type,
    class PropOnCMove = std::true_type,
    class PropOnCSwap = std::true_type
>
using obj_storage_test_t = 
    estd::obj_storage_t<
        storage_size, 
        alignment, 
        Mock::AllocatorMock<uint8_t,PropOnCCopy, PropOnCMove, PropOnCSwap>
    >;
    
using obj_storage_test = obj_storage_test_t<>;


class AllocationTest : public ::testing::TestWithParam<size_t> {
public:
    AllocationTest() : s(GetParam()) {}
protected:
    estd::obj_storage s;
};

template<size_t N>
struct Size {
    static constexpr size_t value = N;
};

// Param 0 - storage size
// Param 1 - alignment size
// Param 2 - PropOnCCopy 
// Param 3 - PropOnCMove 
// Param 4 - PropOnCSwap 
// Param 5 - initial alloc size

// Test Desciptor is a Tuple of the params above
template<class TestDescriptor>
class TestNew : public ::testing::Test {
public:
    static constexpr size_t storage_size = std::decay_t<decltype(std::get<0>(std::declval<TestDescriptor>()))>::value;
    static constexpr size_t alignment = std::decay_t<decltype(std::get<1>(std::declval<TestDescriptor>()))>::value;
    using PropOnCCopy = std::decay_t<decltype(std::get<2>(std::declval<TestDescriptor>()))>;
    using PropOnCMove = std::decay_t<decltype(std::get<3>(std::declval<TestDescriptor>()))>;
    using PropOnCSwap = std::decay_t<decltype(std::get<4>(std::declval<TestDescriptor>()))>;
    static constexpr size_t alloc_size = std::decay_t<decltype(std::get<5>(std::declval<TestDescriptor>()))>::value;

    using storage_t = 
        estd::obj_storage_t<
            storage_size, 
            alignment, 
            Mock::AllocatorMock<uint8_t, PropOnCCopy, PropOnCMove, PropOnCSwap>
        >;

    //TestNew() : s(init_alloc_size) {}
    TestNew() {}


    void TearDown() override {
        using ::testing::_;
        s_allocated_inline.deallocate();
        if (s_allocated.size() > s_allocated.max_size()) {
            EXPECT_CALL(s_allocated.get_allocator(), deallocate(_, _)).Times(1);
            s_allocated.deallocate();
        }
    }


protected:
    storage_t s;
    storage_t s_allocated_inline;
    storage_t s_allocated;
};

TYPED_TEST_CASE_P(TestNew);


template<class TestDescriptor>
class Test : public TestNew<TestDescriptor> {
public:
    static constexpr auto ptr = (storage_t::pointer)0xc0dedead;

    void SetUp() override {
        using ::testing::Return;
        s_allocated_inline.allocate(0);
        EXPECT_CALL(s_allocated.get_allocator(), allocate(s_allocated.max_size() + 1 + alignment)).Times(1).WillOnce(Return(ptr));
        s_allocated.allocate(s_allocated.max_size() + 1);
    }
};

template<class TestDescriptor>
class TestInline : public TestNew<TestDescriptor> {
public:
    static constexpr auto ptr = (storage_t::pointer)0xc0dedead;

    void SetUp() override {
        using ::testing::Return;
        s_allocated_inline.allocate(alloc_size);
        EXPECT_CALL(s_allocated.get_allocator(), allocate(s_allocated.max_size() + 1 + alignment)).Times(1).WillOnce(Return(ptr));
        s_allocated.allocate(s_allocated.max_size()+1);
    }

};

template<class TestDescriptor>
class TestAllocated: public TestNew<TestDescriptor> {
public:
    static constexpr auto ptr = (storage_t::pointer)0xc0dedead;

    void SetUp() override {
        using ::testing::Return;
        s_allocated_inline.allocate(0);
        EXPECT_CALL(s_allocated.get_allocator(), allocate(alloc_size + alignment)).Times(1).WillOnce(Return(ptr));
        s_allocated.allocate(alloc_size);
    }

};


TYPED_TEST_CASE_P(Test);
TYPED_TEST_CASE_P(TestInline);
TYPED_TEST_CASE_P(TestAllocated);

TYPED_TEST_P(TestInline, BasicAllocation)
{

    ASSERT_LE(alloc_size,s.max_size());
    s.allocate(alloc_size);
    EXPECT_NE(nullptr, s.get());

    auto p = reinterpret_cast<uintptr_t>(s.get());
    auto ps = reinterpret_cast<uintptr_t>(&s);
    auto diff = p - ps;
    EXPECT_EQ(
        0,
        reinterpret_cast<uintptr_t>(s.get()) % alignment);
        EXPECT_LT(diff, sizeof(s));
    s.deallocate();
    EXPECT_EQ(0, s.size());
}


TYPED_TEST_P(TestAllocated, BasicAllocation)
{
    using ::testing::_;
    using ::testing::Return;
    constexpr auto ptr = (storage_t::pointer)0xdeadbeef;

    ASSERT_GT(alloc_size, s.max_size());
    EXPECT_CALL(s.get_allocator(), allocate(alloc_size + alignment)).Times(1).WillOnce(Return(ptr));
    
    s.allocate(alloc_size);
    auto diff  = reinterpret_cast<std::uintptr_t>(s.get()) - reinterpret_cast<std::uintptr_t>(ptr);
    
    EXPECT_LT(diff,alignment);
    EXPECT_EQ(0,reinterpret_cast<uintptr_t>(s.get()) % alignment);
    EXPECT_CALL(s.get_allocator(), deallocate(ptr,_));
    
    s.deallocate();
    
    EXPECT_EQ(nullptr, s.get());
    EXPECT_EQ(0, s.size());
}


TYPED_TEST_P(Test, CopyAssignFromInlineToInline) {
    using ::testing::_;
    using ::testing::Return;

    storage_t s2(1);
    if (PropOnCCopy::value) {
        EXPECT_CALL(s2.get_allocator(), move_assign(_));
    }
    s2 = s_allocated_inline;
    EXPECT_EQ(s2.size(), s_allocated_inline.size());
    EXPECT_NE(s2.get(), s_allocated_inline.get());
}

TYPED_TEST_P(Test, CopyAssignFromInlineToAllocated) {
    using ::testing::_;
    using ::testing::Return;


    if (PropOnCCopy::value) {
        EXPECT_CALL(s_allocated.get_allocator(), move_assign(_));
    }
    EXPECT_CALL(s_allocated.get_allocator(), deallocate(ptr,s_allocated.size()));
    s_allocated = s_allocated_inline;
    EXPECT_EQ(s_allocated.size(), s_allocated_inline.size());
    EXPECT_NE(s_allocated.get(), s_allocated_inline.get());
}

REGISTER_TYPED_TEST_CASE_P(Test,
    CopyAssignFromInlineToInline,
    CopyAssignFromInlineToAllocated
    );

REGISTER_TYPED_TEST_CASE_P(TestAllocated,
    BasicAllocation);
REGISTER_TYPED_TEST_CASE_P(TestInline,
    BasicAllocation);


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

TEST_P(AllocationTest, MoveAssignmentInline) {

    estd::obj_storage s_i(1);
    auto old_storage = s.get();
    auto size = s.size();

    if (s.size() <= s.max_size()) {
        s_i = std::move(s);
        EXPECT_EQ(s_i.size(), s.size());
        EXPECT_NE(s_i.get(), s.get());
    }
    else {
        s_i = std::move(s);
        EXPECT_EQ(size, s_i.size());
        EXPECT_EQ(old_storage, s_i.get());
        EXPECT_EQ(nullptr, s.get());
        EXPECT_EQ(0, s.size());
    }
}

TEST_P(AllocationTest, MoveAssignmentAllocated) {

    estd::obj_storage s_a(s.max_size() + 1);
    auto old_storage = s.get();
    auto size = s.size();

    if (s.size() <= s.max_size()) {
        s_a = std::move(s);
        EXPECT_EQ(s_a.size(), s.size());
        EXPECT_NE(s_a.get(), s.get());
    }
    else {
        s_a = std::move(s);
        EXPECT_EQ(size, s_a.size());
        EXPECT_EQ(old_storage, s_a.get());
        EXPECT_EQ(nullptr, s.get());
        EXPECT_EQ(0, s.size());

    }
}

// TODO: mock allocator and add test cases

}  // namespace ObjStorageTest

#endif  // TEST_OBJ_STORAGE_H_
