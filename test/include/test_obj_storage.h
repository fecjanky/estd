#ifndef TEST_OBJ_STORAGE_H_
#define TEST_OBJ_STORAGE_H_

#include <tuple>
#include <cmath>
#include <cstdint>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mock_allocator.h"
#include "memory.h"


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
    estd::sso_storage_t<
        storage_size, 
        alignment, 
        Mock::AllocatorMock<uint8_t,PropOnCCopy, PropOnCMove, PropOnCSwap>
    >;
    
using obj_storage_test = obj_storage_test_t<>;


class AllocationTest : public ::testing::TestWithParam<size_t> {
public:
    AllocationTest() : s(GetParam()) {}
protected:
    estd::sso_storage s;
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
    using Allocator = Mock::AllocatorMock<uint8_t, PropOnCCopy, PropOnCMove, PropOnCSwap>;
    static constexpr uintptr_t ptr = 0xc0dedead;

    using storage_t = 
        estd::sso_storage_t<
            storage_size, 
            alignment, 
            Allocator
        >;

    //TestNew() : s(init_alloc_size) {}
    TestNew() {}


    void TearDown() override {
        using ::testing::_;

        if (s_allocated_inline.size() > s_allocated_inline.max_size()) {
            EXPECT_CALL(s_allocated_inline.get_allocator(), deallocate(_, _)).Times(1);
            s_allocated_inline.deallocate();
        }

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
    using storage_t = typename TestNew<TestDescriptor>::storage_t;

    using PropOnCCopy = typename TestNew<TestDescriptor>::PropOnCCopy;
    using PropOnCMove = typename TestNew<TestDescriptor>::PropOnCMove;
    using PropOnCSwap = typename TestNew<TestDescriptor>::PropOnCSwap;
    static constexpr auto alignment = TestNew<TestDescriptor>::alignment;

    void SetUp() override {
        using ::testing::Return;
        auto pptr = reinterpret_cast<typename storage_t::pointer>
                        (TestNew<TestDescriptor>::ptr);
        this->s_allocated_inline.allocate(0);
        EXPECT_CALL(this->s_allocated.get_allocator(),
                allocate(this->s_allocated.max_size() + 1 + alignment))
            .Times(1)
            .WillOnce(Return(pptr));
        this->s_allocated.allocate(this->s_allocated.max_size() + 1);
    }
};

template<class TestDescriptor>
class TestInlineAllocation : public TestNew<TestDescriptor> {
public:
    using storage_t = typename TestNew<TestDescriptor>::storage_t;
    using PropOnCCopy = typename TestNew<TestDescriptor>::PropOnCCopy;
    using PropOnCMove = typename TestNew<TestDescriptor>::PropOnCMove;
    using PropOnCSwap = typename TestNew<TestDescriptor>::PropOnCSwap;
    static constexpr auto alignment = TestNew<TestDescriptor>::alignment;

    void SetUp() override {
        using ::testing::Return;
        auto pptr = reinterpret_cast<typename storage_t::pointer>
            (TestNew<TestDescriptor>::ptr);
        this->s_allocated_inline.allocate(this->alloc_size);
        EXPECT_CALL(this->s_allocated.get_allocator(),
                allocate(this->s_allocated.max_size() + 1 + alignment))
            .Times(1)
            .WillOnce(Return(pptr));
        this->s_allocated.allocate(this->s_allocated.max_size()+1);
    }

};

template<class TestDescriptor>
class TestAllocation: public TestNew<TestDescriptor> {
public:
    using storage_t = typename TestNew<TestDescriptor>::storage_t;
    using PropOnCCopy = typename TestNew<TestDescriptor>::PropOnCCopy;
    using PropOnCMove = typename TestNew<TestDescriptor>::PropOnCMove;
    using PropOnCSwap = typename TestNew<TestDescriptor>::PropOnCSwap;
    static constexpr auto alignment = TestNew<TestDescriptor>::alignment;
    static constexpr auto alloc_size = TestNew<TestDescriptor>::alloc_size;

    void SetUp() override {
        using ::testing::Return;
        auto pptr = reinterpret_cast<typename storage_t::pointer>
            (TestNew<TestDescriptor>::ptr);

        this->s_allocated_inline.allocate(0);
        EXPECT_CALL(this->s_allocated.get_allocator(),
                allocate(alloc_size + alignment))
            .Times(1)
            .WillOnce(Return(pptr));
        this->s_allocated.allocate(this->alloc_size);
    }

};


TYPED_TEST_CASE_P(Test);
TYPED_TEST_CASE_P(TestInlineAllocation);
TYPED_TEST_CASE_P(TestAllocation);

TYPED_TEST_P(TestInlineAllocation, BasicAllocation)
{
    using BaseT = ::ObjStorageTest::TestNew<TypeParam>;
    constexpr auto alignment = BaseT::alignment;
    constexpr auto alloc_size = BaseT::alloc_size;

    ASSERT_LE(alloc_size,this->s.max_size());
    this->s.allocate(alloc_size);
    EXPECT_NE(nullptr, this->s.get());

    auto p = reinterpret_cast<uintptr_t>(this->s.get());
    auto ps = reinterpret_cast<uintptr_t>(&this->s);
    auto diff = p - ps;
    EXPECT_EQ(
        0U,
        reinterpret_cast<uintptr_t>(this->s.get()) % alignment);
        EXPECT_LT(diff, sizeof(this->s));
    this->s.deallocate();
    EXPECT_EQ(0U, this->s.size());
}


TYPED_TEST_P(TestAllocation, BasicAllocation)
{
    using ::testing::_;
    using ::testing::Return;
    using storage_t = typename TestAllocation<TypeParam>::storage_t;
    auto ptr = (typename storage_t::pointer)0xdeadbeef;
    using BaseT = ::ObjStorageTest::TestNew<TypeParam>;
    constexpr auto alignment = BaseT::alignment;
    constexpr auto alloc_size = BaseT::alloc_size;

    ASSERT_GT(alloc_size, this->s.max_size());
    EXPECT_CALL(this->s.get_allocator(), allocate( alloc_size + alignment))
        .Times(1)
        .WillOnce(Return(ptr));

    this->s.allocate(alloc_size);

    EXPECT_EQ(::estd::impl::aligned_heap_addr(ptr,alignment),this->s.get());
    EXPECT_EQ(0U,reinterpret_cast<uintptr_t>(this->s.get()) % alignment);
    EXPECT_CALL(this->s.get_allocator(), deallocate(ptr,alloc_size + alignment));
    
    this->s.deallocate();
    
    EXPECT_EQ(nullptr, this->s.get());
    EXPECT_EQ(0U, this->s.size());
}


TYPED_TEST_P(Test, CopyAssignFromInlineToInline) {
    using ::testing::_;
    using ::testing::Return;
    using BaseT = ::ObjStorageTest::TestNew<TypeParam>;
    using storage_t = typename BaseT::storage_t;
    using PropOnCCopy =  typename BaseT::PropOnCCopy;

    storage_t s2(1);
    if (PropOnCCopy::value) {
        EXPECT_CALL(s2.get_allocator(), move_assign(_));
    }
    s2 = this->s_allocated_inline;
    EXPECT_EQ(s2.size(), this->s_allocated_inline.size());
    EXPECT_NE(s2.get(), this->s_allocated_inline.get());
}

TYPED_TEST_P(Test, CopyAssignFromInlineToAllocated) {
    using ::testing::_;
    using ::testing::Return;
    using BaseT = ::ObjStorageTest::TestAllocation<TypeParam>;
    using storage_t = typename BaseT::storage_t;
    using PropOnCCopy =  typename BaseT::PropOnCCopy;
    auto ptr = reinterpret_cast<typename storage_t::pointer>(Test<TypeParam>::ptr);

    if (PropOnCCopy::value) {
        EXPECT_CALL(this->s_allocated.get_allocator(), move_assign(_));
    }
    EXPECT_CALL(this->s_allocated.get_allocator(),
            deallocate(ptr,this->s_allocated.size()));
    this->s_allocated = this->s_allocated_inline;
    EXPECT_EQ(this->s_allocated.size(), this->s_allocated_inline.size());
    EXPECT_NE(this->s_allocated.get(), this->s_allocated_inline.get());
}

TYPED_TEST_P(Test, CopyAssignFromAllocatedtoInline) {
    using ::testing::_;
    using ::testing::Return;
    using ::testing::DefaultValue;

    using BaseT = ::ObjStorageTest::TestAllocation<TypeParam>;
    using PropOnCCopy =  typename BaseT::PropOnCCopy;

    if (PropOnCCopy::value) {
        EXPECT_CALL(this->s_allocated_inline.get_allocator(), move_assign(_));
    }
    using Allocator = typename TestNew<TypeParam>::Allocator;
    DefaultValue<typename Allocator::pointer>::Set(Mock::pointer<typename Allocator::value_type>(0xdefdefde));

    this->s_allocated_inline = this->s_allocated;

    DefaultValue<typename Allocator::value_type>::Clear();

    EXPECT_EQ(this->s_allocated.size(), this->s_allocated_inline.size());
    EXPECT_NE(this->s_allocated.get(), this->s_allocated_inline.get());
}

TYPED_TEST_P(Test, CopyAssignFromAllocatedtoAllocated) {
    using ::testing::_;
    using ::testing::Return;
    using ::testing::DefaultValue;
    using ::testing::InSequence;
    using Allocator = typename TestNew<TypeParam>::Allocator;
    using BaseT = ::ObjStorageTest::Test<TypeParam>;
    using PropOnCCopy = typename BaseT::PropOnCCopy;
    using storage_t = typename BaseT::storage_t;
    constexpr auto alignment = BaseT::alignment;
    constexpr auto asize = storage_t::max_size() + 1;
    constexpr auto real_asize = asize + alignment;
    auto test_ptr = Mock::pointer<uint8_t>(0xfefec0c0);
    auto init_ptr = Mock::pointer<uint8_t>(0xdefdefde);

    DefaultValue<typename Allocator::pointer>::
        Set(Mock::pointer<typename Allocator::value_type>(init_ptr));

    storage_t test(asize);

    DefaultValue<typename Allocator::pointer>::
        Set(Mock::pointer<typename Allocator::value_type>(test_ptr));


    if (PropOnCCopy::value) {
        //Allocation is unexpect_callable in temp_allocator
        InSequence d;
        EXPECT_CALL(test.get_allocator(), deallocate(init_ptr, real_asize));
        EXPECT_CALL(test.get_allocator(), move_assign(_));
    } else {
        InSequence d;
        EXPECT_CALL(test.get_allocator(), 
            allocate(this->s_allocated.size()))
            .WillOnce(Return(test_ptr));
        EXPECT_CALL(test.get_allocator(), deallocate(init_ptr, real_asize));
        EXPECT_CALL(test.get_allocator(), move_assign(_)).Times(0);
        EXPECT_CALL(test.get_allocator(), copy_assign(_)).Times(0);
    }
    
    test = this->s_allocated;

    DefaultValue<typename Allocator::value_type>::Clear();

    EXPECT_CALL(test.get_allocator(), deallocate(test_ptr,
        this->s_allocated.size()));

    EXPECT_EQ(this->s_allocated.size(), test.size());
    EXPECT_NE(this->s_allocated.get(), test.get());
    EXPECT_EQ(estd::impl::aligned_heap_addr(test_ptr,alignment),
        test.get());
}

TYPED_TEST_P(Test, MoveAssignFromInlineToInline) {
    using ::testing::_;
    using ::testing::Return;
    using BaseT = ::ObjStorageTest::TestNew<TypeParam>;
    using storage_t = typename BaseT::storage_t;
    using PropOnCMove =  typename BaseT::PropOnCMove;

    auto old_size = this->s_allocated_inline.size();

    storage_t s2(1);

    if (PropOnCMove::value) {
        EXPECT_CALL(s2.get_allocator(), move_assign(_));
    } else {
        EXPECT_CALL(s2.get_allocator(), equals(_));
    }

    s2 = std::move(this->s_allocated_inline);

    EXPECT_EQ(old_size,s2.size());
    EXPECT_EQ(old_size,this->s_allocated_inline.size());
}

TYPED_TEST_P(Test, MoveAssignFromInlineToAllocated) {
    using ::testing::_;
    using ::testing::Return;
    using BaseT = ::ObjStorageTest::TestAllocation<TypeParam>;
    using PropOnCMove =  typename BaseT::PropOnCMove;
    using storage_t = typename BaseT::storage_t;
    auto ptr = reinterpret_cast<typename storage_t::pointer>(Test<TypeParam>::ptr);
    auto old_size = this->s_allocated_inline.size();

    if (PropOnCMove::value) {
        EXPECT_CALL(this->s_allocated.get_allocator(), move_assign(_));
    } else {
        EXPECT_CALL(this->s_allocated.get_allocator(), equals(_));
    }

    EXPECT_CALL(this->s_allocated.get_allocator(),
            deallocate(ptr,this->s_allocated.size()));

    this->s_allocated = std::move(this->s_allocated_inline);

    EXPECT_EQ(old_size,this->s_allocated.size());
    EXPECT_EQ(old_size,this->s_allocated_inline.size());
}

TYPED_TEST_P(Test, MoveAssignFromAllocatedtoInlineComparesEq) {
    using ::testing::_;
    using ::testing::Return;
    using BaseT = ::ObjStorageTest::TestAllocation<TypeParam>;
    using PropOnCMove =  typename BaseT::PropOnCMove;
    auto old_size = this->s_allocated.size();
    auto old_ptr = this->s_allocated.get();

    if (PropOnCMove::value) {
        EXPECT_CALL(this->s_allocated_inline.get_allocator(), move_assign(_));
    } else {
        EXPECT_CALL(this->s_allocated_inline.get_allocator(), equals(_))
                .WillOnce(Return(true));
    }

    this->s_allocated_inline = std::move(this->s_allocated);

    EXPECT_EQ(old_size,this->s_allocated_inline.size());
    EXPECT_EQ(old_ptr,this->s_allocated_inline.get());
    EXPECT_EQ(0U,this->s_allocated.size());
}

TYPED_TEST_P(Test, MoveAssignFromAllocatedtoInlineComparesNEq) {
    using ::testing::_;
    using ::testing::Return;
    using BaseT = ::ObjStorageTest::TestAllocation<TypeParam>;
    using PropOnCMove =  typename BaseT::PropOnCMove;
    auto old_size = this->s_allocated.size();
    auto old_ptr = this->s_allocated.get();
    auto test_ptr = Mock::pointer<uint8_t>(0xfefec0c0);

    if (PropOnCMove::value) {
        EXPECT_CALL(this->s_allocated_inline.get_allocator(), move_assign(_));
    } else {
        EXPECT_CALL(this->s_allocated_inline.get_allocator(), equals(_))
                .WillOnce(Return(false));
        EXPECT_CALL(this->s_allocated_inline.get_allocator(),
                allocate(this->s_allocated.size()))
                .WillOnce(Return(test_ptr));
    }

    this->s_allocated_inline = std::move(this->s_allocated);

    EXPECT_EQ(old_size,this->s_allocated_inline.size());
    if (PropOnCMove::value) {
        EXPECT_EQ(old_ptr,this->s_allocated_inline.get());
        EXPECT_EQ(0U,this->s_allocated.size());
    } else {
        EXPECT_EQ(test_ptr,this->s_allocated_inline.get());
        EXPECT_EQ(old_size,this->s_allocated.size());
    }
}


TYPED_TEST_P(Test, MoveAssignFromAllocatedtoAllocatedComparesEq) {
    using ::testing::_;
    using ::testing::Return;
    using ::testing::DefaultValue;
    using ::testing::InSequence;
    using Allocator = typename TestNew<TypeParam>::Allocator;
    using BaseT = ::ObjStorageTest::Test<TypeParam>;
    using PropOnCMove = typename BaseT::PropOnCMove;
    using storage_t = typename BaseT::storage_t;
    constexpr auto alignment = BaseT::alignment;
    constexpr auto asize = storage_t::max_size() + 1;
    constexpr auto real_asize = asize + alignment;
    auto init_ptr = Mock::pointer<uint8_t>(0xdefdefde);
    auto ptr = reinterpret_cast<typename storage_t::pointer>(Test<TypeParam>::ptr);

    DefaultValue<typename Allocator::pointer>::
        Set(Mock::pointer<typename Allocator::value_type>(init_ptr));

    storage_t test(asize);

    DefaultValue<typename Allocator::value_type>::Clear();

    auto old_size = this->s_allocated.size();

    EXPECT_CALL(test.get_allocator(), deallocate(ptr,old_size));
    EXPECT_CALL(test.get_allocator(), deallocate(init_ptr, real_asize));

    if (PropOnCMove::value) {
        //Allocation is unexpect_callable in temp_allocator
        InSequence d;
        EXPECT_CALL(test.get_allocator(), move_assign(_));
    } else {
        InSequence d;
        EXPECT_CALL(test.get_allocator(), equals(_))
                .WillOnce(Return(true));
    }

    test = std::move(this->s_allocated);

    EXPECT_EQ(old_size, test.size());
    EXPECT_EQ(0U, this->s_allocated.size());
    EXPECT_EQ(estd::impl::aligned_heap_addr(ptr,alignment),
        test.get());
}


TYPED_TEST_P(Test, MoveAssignFromAllocatedtoAllocatedComparesNEq) {
    using ::testing::_;
    using ::testing::Return;
    using ::testing::DefaultValue;
    using ::testing::InSequence;
    using Allocator = typename TestNew<TypeParam>::Allocator;
    using BaseT = ::ObjStorageTest::Test<TypeParam>;
    using PropOnCMove = typename BaseT::PropOnCMove;
    using storage_t = typename BaseT::storage_t;
    constexpr auto alignment = BaseT::alignment;
    constexpr auto asize = storage_t::max_size() + 1;
    constexpr auto real_asize = asize + alignment;
    auto init_ptr = Mock::pointer<uint8_t>(0xdefdefde);
    auto ptr = reinterpret_cast<typename storage_t::pointer>(Test<TypeParam>::ptr);
    auto test_ptr = Mock::pointer<uint8_t>(0xfefec0c0);

    DefaultValue<typename Allocator::pointer>::
        Set(Mock::pointer<typename Allocator::value_type>(init_ptr));

    storage_t test(asize);

    DefaultValue<typename Allocator::value_type>::Clear();

    auto old_size = this->s_allocated.size();



    if (PropOnCMove::value) {
        EXPECT_CALL(test.get_allocator(), deallocate(ptr,old_size));
        EXPECT_CALL(test.get_allocator(), deallocate(init_ptr, real_asize));
        //Allocation is unexpect_callable in temp_allocator
        EXPECT_CALL(test.get_allocator(), move_assign(_));
    } else {
        EXPECT_CALL(test.get_allocator(), deallocate(init_ptr,old_size));
        EXPECT_CALL(test.get_allocator(), deallocate(test_ptr, real_asize));

        EXPECT_CALL(test.get_allocator(), equals(_))
                .WillOnce(Return(false));

        EXPECT_CALL(test.get_allocator(), allocate(old_size))
                .WillOnce(Return(test_ptr));
    }

    test = std::move(this->s_allocated);

    EXPECT_EQ(old_size, test.size());
    if (PropOnCMove::value) {
        EXPECT_EQ(0U, this->s_allocated.size());
        EXPECT_EQ(estd::impl::aligned_heap_addr(ptr,alignment),
            test.get());
    } else {
        EXPECT_EQ(old_size, this->s_allocated.size());
        EXPECT_EQ(estd::impl::aligned_heap_addr(test_ptr,alignment),
            test.get());
        EXPECT_EQ(estd::impl::aligned_heap_addr(ptr,alignment),
                this->s_allocated.get());
    }
}



TYPED_TEST_P(Test, SwapFromInlineToInline) {
    using ::testing::_;
    using ::testing::Return;
    using ::testing::ByRef;
    using BaseT = ::ObjStorageTest::TestAllocation<TypeParam>;
    using storage_t = typename BaseT::storage_t;
    using PropOnCSwap = typename BaseT::PropOnCSwap;

    storage_t t(storage_t::max_size());
    auto i_size = this->s_allocated_inline.size();
    ASSERT_NE(t.size(), i_size);

    if (PropOnCSwap::value) {
        EXPECT_CALL(t.get_allocator(), swap_object(_));
    }
    else {
        EXPECT_CALL(t.get_allocator(), equals(_))
            .WillOnce(Return(true));
    }
    using std::swap;
    swap(t,this->s_allocated_inline);

    EXPECT_EQ(i_size, t.size());
    EXPECT_EQ(storage_t::max_size(), this->s_allocated_inline.size());
}

TYPED_TEST_P(Test, SwapFromInlineToAllocated) {
    using ::testing::_;
    using ::testing::Return;
    using ::testing::ByRef;
    using BaseT = ::ObjStorageTest::TestAllocation<TypeParam>;
    using PropOnCSwap = typename BaseT::PropOnCSwap;

    auto i_size = this->s_allocated_inline.size();
    auto a_size = this->s_allocated.size();
    auto ptr = this->s_allocated.get();

    if (PropOnCSwap::value) {
        EXPECT_CALL(this->s_allocated.get_allocator(), swap_object(_)).Times(2);
    }
    else {
        EXPECT_CALL(this->s_allocated.get_allocator(), equals(_))
            .WillRepeatedly(Return(true));
    }

    using std::swap;
    swap(this->s_allocated, this->s_allocated_inline);

    EXPECT_EQ(i_size, this->s_allocated.size());
    EXPECT_EQ(a_size, this->s_allocated_inline.size());
    EXPECT_EQ(ptr, this->s_allocated_inline.get());
    EXPECT_NE(ptr, this->s_allocated.get());

    swap(this->s_allocated, this->s_allocated_inline);

    EXPECT_EQ(a_size, this->s_allocated.size());
    EXPECT_EQ(i_size, this->s_allocated_inline.size());
    EXPECT_EQ(ptr, this->s_allocated.get());
    EXPECT_NE(ptr, this->s_allocated_inline.get());
}

TYPED_TEST_P(Test, SwapFromAllocatedToAllocatedComparesEq) {
    using ::testing::_;
    using ::testing::Return;
    using ::testing::Ge;
    using BaseT = ::ObjStorageTest::TestAllocation<TypeParam>;
    using storage_t = typename BaseT::storage_t;
    using PropOnCSwap = typename BaseT::PropOnCSwap;
    auto test_ptr = Mock::pointer<uint8_t>(0xfefec0c0);
    constexpr auto alloc_size = storage_t::max_size() * 10;

    storage_t t;

    EXPECT_CALL(t.get_allocator(), allocate(Ge(alloc_size)))
        .WillOnce(Return(test_ptr));
    EXPECT_CALL(t.get_allocator(), deallocate(_, _));

    t.allocate(alloc_size);

    auto t_size = t.size();
    auto a_size = this->s_allocated.size();
    auto ptr = this->s_allocated.get();
    auto t_ptr = t.get();

    if (PropOnCSwap::value) {
        EXPECT_CALL(this->s_allocated.get_allocator(), swap_object(_));
    }
    else {
        EXPECT_CALL(this->s_allocated.get_allocator(), equals(_))
            .WillRepeatedly(Return(true));
    }

    swap(this->s_allocated, t);

    EXPECT_EQ(t_size, this->s_allocated.size());
    EXPECT_EQ(a_size,t.size());
    EXPECT_EQ(ptr, t.get());
    EXPECT_EQ(t_ptr, this->s_allocated.get());
}

TYPED_TEST_P(Test, SwapFromAllocatedToAllocatedComparesNEq) {
    using ::testing::_;
    using ::testing::Return;
    using ::testing::Ge;
    using BaseT = ::ObjStorageTest::TestAllocation<TypeParam>;
    using storage_t = typename BaseT::storage_t;
    using PropOnCSwap = typename BaseT::PropOnCSwap;
    auto test_ptr = Mock::pointer<uint8_t>(0xfefec0c0);
    constexpr auto alloc_size = storage_t::max_size() * 10;

    storage_t t;

    EXPECT_CALL(t.get_allocator(), allocate(Ge(alloc_size)))
        .WillOnce(Return(test_ptr));
    EXPECT_CALL(t.get_allocator(), deallocate(_, _));

    t.allocate(alloc_size);

    auto t_size = t.size();
    auto a_size = this->s_allocated.size();
    auto ptr = this->s_allocated.get();
    auto t_ptr = t.get();
    using std::swap;
    if (PropOnCSwap::value) {
        EXPECT_CALL(this->s_allocated.get_allocator(), swap_object(_));
        swap(this->s_allocated, t);
        EXPECT_EQ(t_size, this->s_allocated.size());
        EXPECT_EQ(a_size, t.size());
        EXPECT_EQ(ptr, t.get());
        EXPECT_EQ(t_ptr, this->s_allocated.get());
    }
    else {
        EXPECT_CALL(this->s_allocated.get_allocator(), equals(_))
            .WillRepeatedly(Return(false));
        EXPECT_THROW(swap(this->s_allocated, t),std::exception);
    }
}

REGISTER_TYPED_TEST_CASE_P(Test,
    CopyAssignFromInlineToInline,
    CopyAssignFromInlineToAllocated,
    CopyAssignFromAllocatedtoInline,
    CopyAssignFromAllocatedtoAllocated,
    MoveAssignFromInlineToInline,
    MoveAssignFromInlineToAllocated,
    MoveAssignFromAllocatedtoInlineComparesEq,
    MoveAssignFromAllocatedtoInlineComparesNEq,
    MoveAssignFromAllocatedtoAllocatedComparesEq,
    MoveAssignFromAllocatedtoAllocatedComparesNEq,
    SwapFromInlineToInline,
    SwapFromInlineToAllocated,
    SwapFromAllocatedToAllocatedComparesEq,
    SwapFromAllocatedToAllocatedComparesNEq
    );

REGISTER_TYPED_TEST_CASE_P(TestAllocation,
    BasicAllocation);
REGISTER_TYPED_TEST_CASE_P(TestInlineAllocation,
    BasicAllocation);

/////////////////////////////////////////////

TEST_P(AllocationTest, CopyConstruction) {

    estd::sso_storage s2 = s;
    EXPECT_EQ(s2.get_allocator(), s.get_allocator());
    EXPECT_EQ(s2.size(), s.size());
    ASSERT_EQ(s2.max_size(), s.max_size());
    EXPECT_NE(s2.get(), s.get());
    
}

TEST_P(AllocationTest, MoveConstruction) {

    auto old_storage = s.get();
    estd::sso_storage s2 = std::move(s);
    if (s2.size() <= s2.max_size()) {
        EXPECT_EQ(s2.size(), s.size());
        EXPECT_NE(s2.get(), s.get());
    } else {
        EXPECT_EQ(old_storage, s2.get());
    }
}

TEST_P(AllocationTest, CopyAssigmentInline) {

    estd::sso_storage s2(1);
    s2 = s;
    EXPECT_EQ(s2.get_allocator(), s.get_allocator());
    EXPECT_EQ(s2.size(), s.size());
    EXPECT_NE(s2.get(), s.get());
}

TEST_P(AllocationTest, CopyAssigmentAllocated) {

    estd::sso_storage s2(s.max_size()+1);
    s2 = s;
    EXPECT_EQ(s2.get_allocator(), s.get_allocator());
    EXPECT_EQ(s2.size(), s.size());
    EXPECT_NE(s2.get(), s.get());
}

TEST_P(AllocationTest, MoveAssignmentInline) {

    estd::sso_storage s_i(1);
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
        EXPECT_EQ(0U, s.size());
    }
}

TEST_P(AllocationTest, MoveAssignmentAllocated) {

    estd::sso_storage s_a(s.max_size() + 1);
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
        EXPECT_EQ(0U, s.size());

    }
}

}  // namespace ObjStorageTest

#endif  // TEST_OBJ_STORAGE_H_
