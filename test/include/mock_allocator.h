#ifndef MOCK_ALLOCATOR_H_
#define MOCK_ALLOCATOR_H_

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <type_traits>

namespace Mock {


    template<typename T,typename TT>
    constexpr T* pointer(TT t) {
        return reinterpret_cast<T*>(t);
    }

    template<
        typename T
    >
    class AllocatorBase {
    public:

        AllocatorBase() : def_constructed{true},
            copy_constructed{}, move_constructed{}
        {
        }

        AllocatorBase(const AllocatorBase&) :
            def_constructed{},
            copy_constructed{true}, move_constructed{}
        {}

        AllocatorBase(AllocatorBase&&) :
            def_constructed{},
            copy_constructed{}, move_constructed{ true }
        {}

        template<
            typename A,
            typename = std::enable_if_t<
                std::is_constructible<AllocatorBase,std::decay_t<A>>::value
            >
        > AllocatorBase(A&& a) : def_constructed{},
            copy_constructed{std::is_lvalue_reference<A>::value}, 
            move_constructed{!std::is_lvalue_reference<A>::value } 
        {
        }

        AllocatorBase& operator=(const AllocatorBase&) noexcept = default;
        AllocatorBase& operator=(AllocatorBase&&) noexcept = default;

        const bool def_constructed;
        const bool copy_constructed;
        const bool move_constructed;
    };

    template<
        typename T,
        class PropOnCCopy = std::true_type,
        class PropOnCMove = std::true_type,
        class PropOnCSwap = std::true_type
    > class AllocatorMock : public AllocatorBase<T> {
    public:
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using const_pointer = const pointer;
        using const_reference = const T&;
        using size_type = size_t;

        using propagate_on_container_copy_assignment = PropOnCCopy;
        using propagate_on_container_move_assignment = PropOnCMove;
        using propagate_on_container_swap = PropOnCSwap;

        AllocatorMock() {}
        
        AllocatorMock(const AllocatorMock& a) : AllocatorBase<T>(a){}
        AllocatorMock(AllocatorMock&& a) noexcept : AllocatorBase<T>(std::move(a)) {}

        template<typename TT>
        AllocatorMock(const AllocatorMock<TT, PropOnCCopy, PropOnCMove, PropOnCSwap>& a) : AllocatorBase<T>(a) {}

        template<typename TT>
        AllocatorMock(AllocatorMock<TT, PropOnCCopy, PropOnCMove, PropOnCSwap>&& a) : AllocatorBase<T>(std::move(a)) {}

        AllocatorMock& operator=(const AllocatorMock& a) noexcept {
            copy_assign(a);
            return *this;
        }

        AllocatorMock& operator=(AllocatorMock&& a) noexcept {
            move_assign(std::move(a));
            return *this;
        }

        template<typename TT>
        bool operator==(const AllocatorMock<TT,PropOnCCopy, PropOnCMove, PropOnCSwap>& rhs) const noexcept
        {
            AllocatorMock temp(rhs);
            return equals(rhs);
        }

        template<typename TT>
        bool operator!=(const AllocatorMock<T,PropOnCCopy, PropOnCMove, PropOnCSwap>& rhs) const noexcept
        {
            AllocatorMock temp(rhs);
            return not_equals(rhs);
        }

        template< class U >
        void destroy(U* p) {
            destroy_impl(p);
        }

        //////////////////////
        /// Mocked methods
        //////////////////////
        MOCK_METHOD1_T(allocate, pointer(size_type));
        MOCK_METHOD2_T(allocate, pointer(size_type, const void*));
        MOCK_METHOD2_T(deallocate, void(pointer, size_type));
        MOCK_CONST_METHOD1_T(address, const_pointer(const_reference));
        //MOCK_CONST_METHOD1_T(address, pointer(reference));
        //MOCK_CONST_METHOD1_T(max_size, size_type (void) );
        // TODO(fecjanky): find a way to add support for construct

        // Helpers
        MOCK_METHOD1_T(swap_object, void(AllocatorMock&));
        MOCK_METHOD1_T(copy_assign, void(const AllocatorMock&));
        MOCK_METHOD1_T(move_assign, void(const AllocatorMock&));
        MOCK_CONST_METHOD1_T(equals, bool(const AllocatorMock&));
        MOCK_CONST_METHOD1_T(not_equals, bool(const AllocatorMock&));
        MOCK_METHOD1_T(destroy_impl, void(void*));


    };

    template<typename T,class C,class M, class S>
    inline void swap(Mock::AllocatorMock<T,C,M,S>& lhs, Mock::AllocatorMock<T, C, M, S>& rhs) noexcept
    {
        lhs.swap_object(rhs);
    }

}  // namespace Mock

#endif  // MOCK_ALLOCATOR_H_
