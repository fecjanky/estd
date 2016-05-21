// Copyright (c) 2016 Ferenc Nandor Janky
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef MEMORY_H_
#define MEMORY_H_

#include <cstdint>
#include <array>
#include <memory>
#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <utility>

namespace estd {

namespace impl {
template<size_t P, size_t N>
struct is_power_of;

template<size_t P, size_t N>
struct is_power_of {
    static constexpr size_t remainder = N % P;
    static_assert( N == 1 ? true : remainder == 0, "N is not a power of P");
    static constexpr bool value = is_power_of<P, N / P>::value;
};

template<size_t P>
struct is_power_of<P, 1> {
    static constexpr bool value = true;
};

template<size_t P>
struct is_power_of<P, 0> {
    static constexpr bool value = false;
};

template<typename... Ts>
struct AndType;

template<typename... Ts>
struct AndType<::std::true_type, Ts...> {
    static constexpr bool value = AndType<Ts...>::value;
    using type = typename AndType<Ts...>::type;
};

template<typename... Ts>
struct AndType<::std::false_type , Ts...> {\
    static constexpr bool value = false;
    using type = ::std::false_type;
};

template<>
struct AndType<> {
    static constexpr bool value = true;
    using type = ::std::true_type;
};

template<typename... Ts>
struct OrType;

template<typename... Ts>
struct OrType<::std::true_type, Ts...> {
    static constexpr bool value = true;
    using type = ::std::true_type;
};

template<typename... Ts>
struct OrType<::std::false_type, Ts...> {
    static constexpr bool value = OrType<Ts...>::value;
    using type = typename OrType<Ts...>::type;
};

template<>
struct OrType<> {
    static constexpr bool value = false;
    using type = ::std::false_type;
};

template<typename T>
struct NotType;

template<>
struct NotType<std::true_type> {
    static constexpr bool value = false;
    using type = std::false_type;
};

template<>
struct NotType<std::false_type> {
    static constexpr bool value = true;
    using type = std::true_type;
};


template<typename... T>
using AndType_t = typename AndType<T...>::type;

template<typename... T>
using OrType_t = typename OrType<T...>::type;

template<typename T>
using NotType_t = typename NotType<T>::type;

template<typename A, bool b>
struct select_always_eq_trait {
    using type = typename A::is_always_equal;
};

template<typename A>
struct select_always_eq_trait<A,false> {
    using type = typename std::is_empty<A>::type;
};

// until c++17...
template<typename A>
struct allocator_is_always_equal {
private:
    template<typename AA>
    static std::true_type has_always_equal_test(const AA&,typename AA::is_always_equal*);
    static std::false_type has_always_equal_test(...);
    static constexpr bool has_always_equal = 
        std::is_same<
            std::true_type, 
            decltype(has_always_equal_test(std::declval<A>(),nullptr))
        >::value;
public:
    using type = typename select_always_eq_trait<A, has_always_equal>::type;
    static constexpr bool value = type::value;
};


template<typename A>
using allocator_is_always_equal_t =
        typename allocator_is_always_equal<A>::type;

struct DefaultCloningPolicy {
    template<typename T>
    static T* Clone(const T& from, void* to)
    {
        return from.clone(to);
    }

    template<typename T, typename = ::std::enable_if_t<
            !::std::is_lvalue_reference<T>::value>>
    static T* Move(T&& from, void* to) noexcept
    {
        static_assert(noexcept(std::declval<T>().move(to)),
                "T is not noexcept moveable");
        return from.move(to);
    }
};

template<size_t N>
struct alignment_t {
};

template<typename A>
struct DellocatorDeleter{
    explicit DellocatorDeleter(A& a,size_t size = 1) : a_{a},size_{size}{}
    void operator()(void* ptr){
        a_.deallocate(ptr,size_);
    }
    A& a_;
    size_t size_;
};


static inline void* aligned_heap_addr(void* ptr,size_t alignment) noexcept
{
    return reinterpret_cast<void*>(((reinterpret_cast<uintptr_t>(ptr)+ alignment - 1)/alignment)
                    *alignment);
}

}  //namespace impl

template<
    size_t storage_size = 4, // in pointer size
    size_t alignment_ = alignof(::std::max_align_t),
    class Allocator = ::std::allocator<uint8_t>
>
class sso_storage_t: private Allocator {
public:

    static constexpr size_t max_size_ =
            storage_size < 1 ? sizeof(void*) : storage_size * sizeof(void*);

    static constexpr size_t alignment = alignment_;

    static_assert(impl::is_power_of<2,alignment>::value,"alignment is not a power of 2");

    static_assert(::std::is_same<typename Allocator::value_type, uint8_t>::value,
        "obj_storage_t requires a byte allocator");

    using allocator_type = Allocator;
    using allocator_traits = ::std::allocator_traits<allocator_type>;
    using propagate_on_container_move_assignment = 
        typename allocator_traits::propagate_on_container_move_assignment;
    using propagate_on_container_copy_assignment = 
        typename allocator_traits::propagate_on_container_copy_assignment;
    using propagate_on_container_swap = 
        typename allocator_traits::propagate_on_container_swap;

    using pocma = propagate_on_container_move_assignment;
    using pocca = propagate_on_container_copy_assignment;
    using pocs = propagate_on_container_swap;
    using pointer = typename allocator_traits::pointer;
    static constexpr bool move_assign_noexcept =
            impl::OrType_t<pocma,impl::allocator_is_always_equal_t<allocator_type>>::value;
    static constexpr bool swap_noexcept =
            impl::OrType_t<pocs,impl::allocator_is_always_equal_t<allocator_type>>::value;

    sso_storage_t() :
            storage { }, heap_storage{}, size_ { }
    {
    }

    template<typename A> 
    sso_storage_t(::std::allocator_arg_t, A&& a) :
        Allocator(::std::forward<A>(a)), storage { }, heap_storage{}, size_ { }
    {
    }
    
    explicit sso_storage_t(size_t n) :
            storage { }, heap_storage{},size_ { }
    {
        allocate(n);
    }

    sso_storage_t(const sso_storage_t& rhs) :
        Allocator(
                allocator_traits::select_on_container_copy_construction(
                            rhs.get_allocator())), storage { },heap_storage{}, size_ { }
    {
        if (rhs.size() > 0 && rhs.size() <= rhs.max_size()) {
            allocate_inline(rhs.size());
        } else if (rhs.size() > 0) {
            allocate_with_allocator(rhs.size());
        }
    }

    sso_storage_t(sso_storage_t&& rhs) noexcept
    : Allocator(::std::move(rhs)), storage{}, heap_storage {}, size_ {}
    {
        static_assert(::std::is_nothrow_move_constructible<allocator_type>::value,
            "allocator_type is not nothrow_move_constructible");
        obtain_rvalue(::std::move(rhs));
    }

    sso_storage_t& operator= (const sso_storage_t& rhs)
    {
        return copy_assign_impl(rhs, pocca{});
    }

    sso_storage_t& operator= (sso_storage_t&& rhs) noexcept(move_assign_noexcept)
    {
        return move_assign_impl(::std::move(rhs), pocma{});
    }

    void swap_object(sso_storage_t& rhs) noexcept(swap_noexcept)
    {
        swap_impl(rhs,pocs{});
    }

    void* allocate(size_t n)
    {
        allocation_check();
        n = n == 0 ? 1 : n;
        if(n <= max_size_) {
            return allocate_inline(n);
        } else {
            return allocate_with_allocator_aligned(n);
        }
    }

    void deallocate() noexcept
    {
        if (size_ > max_size_) {
            this->allocator_type::deallocate(heap_storage, size_);
            heap_storage = nullptr;
        }
        size_ = 0;
    }

    ~sso_storage_t()
    {
        deallocate();
    }

    operator bool ()const noexcept
    {
        return size_ != 0;
    }

    size_t size()const noexcept
    {
        return size_;
    }

    static constexpr size_t max_size()
    {
        return max_size_;
    }

    void* get_checked()
    {
        if(size_ == 0) {
            throw ::std::runtime_error("accessing unallocated storage");
        }
        return get();
    }

    void* get() noexcept
    {
        if (size_ > 0 && size_ <= max_size_) {
            return &*storage.begin();
        } else if (size_ > max_size_){
            return impl::aligned_heap_addr(heap_storage,alignment);
        } else {
            return nullptr;
        }
    }

    template<size_t N>
    static constexpr bool is_alignment_ok(const impl::alignment_t<N>&)
    {
        return N <= alignment && impl::is_power_of<2, N>::value;
    }

    const allocator_type& get_allocator()const noexcept
    {
        return *this;
    }

    allocator_type& get_allocator() noexcept
    {
        return *this;
    }

private:

    sso_storage_t& copy_assign_impl(const sso_storage_t& rhs, ::std::true_type)
    {
        static_assert(::std::is_nothrow_move_assignable<allocator_type>::value,
            "allocator_type is not nothrow_move_assignable");

        if (this != &rhs) {
            // make allocator based allocation first to
            // provide strong exception guarantee
            pointer p{};
            allocator_type temp_alloc = rhs.get_allocator();
            if (rhs.size() > rhs.max_size()) {
                p = temp_alloc.allocate(rhs.size());
            }
            deallocate();
            get_allocator() = ::std::move(temp_alloc);
            assign_storage(p, rhs.size());
        }
        return *this;
    }

    sso_storage_t& copy_assign_impl(const sso_storage_t& rhs, ::std::false_type)
    {
        if (this != &rhs) {
            strong_copy_allocation(rhs);
        }
        return *this;
    }

    // move assign if propagate_on_container_move_assignment is true
    sso_storage_t& move_assign_impl(sso_storage_t&& rhs, ::std::true_type) noexcept
    {
        static_assert(::std::is_nothrow_move_assignable<allocator_type>::value,
            "allocator_type is not nothrow_move_assignable");

        if (this != &rhs) {
            deallocate_and_obtain_rvalue(::std::move(rhs));
            get_allocator() = ::std::move(rhs.get_allocator());
        }
        return *this;
    }

    sso_storage_t& move_assign_impl(sso_storage_t&& rhs, ::std::false_type)
        noexcept(impl::allocator_is_always_equal_t<allocator_type>::value)
    {
        return move_assign_impl(::std::move(rhs),::std::false_type{},
                impl::allocator_is_always_equal_t<allocator_type>{});
    }

    // move assign if propagate_on_container_move_assignment is false but
    // allocator is always equal
    sso_storage_t& move_assign_impl(
            sso_storage_t&& rhs, ::std::false_type, ::std::true_type) noexcept
    {
        if (this != &rhs) {
            deallocate_and_obtain_rvalue(::std::move(rhs));
        }
        return *this;
    }

    // move assign if propagate_on_container_move_assignment is false but
    // allocator is not always equal
    sso_storage_t& move_assign_impl(
            sso_storage_t&& rhs,  ::std::false_type,::std::false_type)
    {
        if (this != &rhs) {
            // can obtain storage only if allocators compare to equal
            if (get_allocator() == rhs.get_allocator()) {
                deallocate_and_obtain_rvalue(::std::move(rhs));
            }
            // need to reallocate with this allocator if not equal
            else {
                strong_copy_allocation(rhs);
            }
        }
        return *this;
    }


    void strong_copy_allocation(const sso_storage_t& rhs)
    {
        // make allocator based allocation first to
        // provide strong exception guarantee
        pointer p{};
        if (rhs.size() > rhs.max_size()) {
            p = get_allocator().allocate(rhs.size());
        }
        deallocate();
        assign_storage(p, rhs.size());
    }

    void swap_impl(sso_storage_t& rhs, ::std::true_type) noexcept
    {
        using std::swap;
        swap(get_allocator(),rhs.get_allocator());
        swap_guts(rhs);
    }

    void swap_impl(sso_storage_t& rhs, ::std::false_type)
        noexcept(impl::allocator_is_always_equal_t<allocator_type>::value)
    {
        // dispatch further based on allocator
        swap_impl(rhs,std::false_type{},
               impl::allocator_is_always_equal_t<allocator_type>{});
    }

    void swap_impl(sso_storage_t& rhs, ::std::false_type, ::std::false_type)
    {
        if(!(get_allocator() == rhs.get_allocator())) {
            throw ::std::runtime_error(
                    "obj_storage_t: swap attempt with unequal allocators ");
        }
        swap_guts(rhs);
    }

    void swap_impl(sso_storage_t& rhs, ::std::false_type,std::true_type) noexcept
    {
        swap_guts(rhs);
    }

    void swap_guts(sso_storage_t& rhs) noexcept{
        using std::swap;
        swap(heap_storage,rhs.heap_storage);
        swap(size_,rhs.size_);
    }

    // precondition: this storage is in deallocated state
    void* allocate_inline(size_t n) noexcept
    {
        size_ = n;
        return &*storage.begin();
    }

    // precondition: this storage is in deallocated state
    void* allocate_with_allocator_aligned(size_t n)
    {
        // allocating +alignment bytes to be able to
        // align heap storage as well
        allocate_with_allocator(n + alignment);
        return impl::aligned_heap_addr(heap_storage,alignment);
    }

    // precondition: this storage is in deallocated state
    void* allocate_with_allocator(size_t n)
    {
        // allocating +alignment bytes to be able to
        // align heap storage as well
        heap_storage = get_allocator().allocate(n);
        size_ = n;
        return heap_storage;
    }

    void allocation_check()
    {
        if (size_ != 0) {
            throw ::std::bad_alloc {};
        }
    }

    // precondition: this storage is in deallocated state
    void obtain_rvalue(sso_storage_t&& rhs) noexcept
    {
        // leave rhs as is if inline allocation happens
        if (rhs.size() > 0 && rhs.size() <= max_size_) {
            allocate_inline(rhs.size());
        }
        else if (rhs.size() > 0) {
            swap_guts(rhs);
        }
    }

    void deallocate_and_obtain_rvalue(sso_storage_t&& rhs) noexcept
    {
        deallocate();
        obtain_rvalue(std::move(rhs));
    }

    // precondition: this storage is in deallocated state
    void assign_storage(pointer p,const size_t size) noexcept 
    {
        // use preallocated storage 
        if (size > 0 && size <= max_size()) {
            allocate_inline(size);
        }
        // use allocated storage else
        else if (size > 0) {
            heap_storage = p;
            size_ = size;
        }
    }

    ///////////////
    // Data members
    ///////////////

    ::std::array<::std::uint8_t,max_size_> storage alignas(alignment);
    // storing in union was considered, but this results in cleaner
    // and more correct swap logic
    ::std::uint8_t* heap_storage;
    size_t size_;
};

using sso_storage = sso_storage_t<>;

template<size_t s, size_t a, class A>
bool operator==(const sso_storage_t<s, a, A>& lhs,
        const sso_storage_t<s, a, A>& rhs) noexcept
{
    return lhs.size() == rhs.size() && lhs.get_allocator() == lhs.get_allocator();
}

template<size_t s, size_t a, class A>
bool operator!=(const sso_storage_t<s, a, A>& lhs,
        const sso_storage_t<s, a, A>& rhs) noexcept
{
    return !(lhs == rhs);
}

template<
    class IF,
    typename CloningPolicy = impl::DefaultCloningPolicy,
    size_t storage_size = 4,  // in pointer size
    size_t alignment = alignof(::std::max_align_t),
    class Allocator = ::std::allocator<uint8_t>
>
class polymorphic_obj_storage_t {
public:
    using storage_t = sso_storage_t<storage_size, alignment,Allocator>;

    using allocator_type = typename storage_t::allocator_type;
    using allocator_traits = ::std::allocator_traits<allocator_type>;
    using type = IF;
    static_assert(::std::is_polymorphic<type>::value, "IF class is not polymorphic");
    static_assert(::std::is_same<type, ::std::decay_t<type>>::value, "IF class must be cv unqualifed and non-reference type");
    //static_assert(noexcept(CloningPolicy::Move(::std::move(::std::declval<type>()), ::std::declval<void*>())),
    //    "IF type is not noexcept moveable");

    template<
        typename T,
        typename = ::std::enable_if_t<::std::is_base_of<type, ::std::decay_t<T>>::value>
    >
    explicit polymorphic_obj_storage_t(T&& t) :
        storage { sizeof(::std::decay_t<T>) },
        // using placement new for construction
        obj { ::new (storage.get()) ::std::decay_t<T>(std::forward<T>(t)) }
    {
        static_assert( storage_t::is_alignment_ok(
                        impl::alignment_t<alignof(::std::decay_t<T>)> {}),
                "T is not properly aligned");
    }

    template<
        typename A,
        typename T,
        typename = ::std::enable_if_t<::std::is_base_of<type, ::std::decay_t<T>>::value>
    >
    explicit polymorphic_obj_storage_t(std::allocator_arg_t,A&& a, T&& t) :
        storage { std::allocator_arg,std::forward<A>(a), sizeof(::std::decay_t<T>) },
        // using placement new for construction
        obj { ::new (storage.get()) ::std::decay_t<T>(std::forward<T>(t)) }
    {
        static_assert( storage_t::is_alignment_ok(
                        impl::alignment_t<alignof(::std::decay_t<T>)> {}),
                "T is not properly aligned");
    }

    polymorphic_obj_storage_t() noexcept:
            storage { }, obj { }
    {
    }

    polymorphic_obj_storage_t(const polymorphic_obj_storage_t& rhs) :
            storage { rhs.storage },
            obj { rhs.get() ?
                    CloningPolicy::Clone(*rhs.get(), storage.get()) :
                    nullptr }
    {
    }

    polymorphic_obj_storage_t(polymorphic_obj_storage_t&& rhs) noexcept
    : storage { ::std::move(rhs.storage)}, obj {rhs.obj}
    {
        // successful storage swap, reset rhs obj pointer
        if (storage.size() > storage.max_size()) {
            rhs.obj = nullptr;
        }
        // need to move object if inline allocation happened
        // and rhs has an object
        else if (rhs) {
            obj = CloningPolicy::Move(::std::move(*rhs.get()), storage.get());
        }
    }

    // provides basic guarantee
    polymorphic_obj_storage_t& operator=(const polymorphic_obj_storage_t& rhs)
    {
        if (this != &rhs) {
            cleanup();
            storage = rhs.storage;
            obj = rhs.get() ?
                CloningPolicy::Clone(*rhs.get(), storage.get()) : nullptr;
        }
        return *this;
    }
    
    // strong guarantee if storage is nothrow move assignable
    // else basic guarantee 
    polymorphic_obj_storage_t& operator=(polymorphic_obj_storage_t&& rhs)
        noexcept(::std::is_nothrow_move_assignable<storage_t>::value)
    {
        if (this != &rhs) {
            cleanup();
            // inline allocation case
            if (rhs.storage.size() > 0 &&
                rhs.storage.size() <= rhs.storage.max_size()) {
                storage = ::std::move(rhs.storage);
                obj = CloningPolicy::Move(::std::move(*rhs.get()),
                        storage.get());
            } else if (rhs.storage.size() > 0) {
                move_assign_w_allocated_obj(::std::move(rhs),
                    ::std::is_nothrow_move_assignable<storage_t>{});
            }
            else {
                storage = ::std::move(rhs.storage);
            }
        }
        return *this;
    }

    void swap_object(polymorphic_obj_storage_t& rhs)
        noexcept(noexcept(std::declval<storage_t&>().swap_object(std::declval<storage_t&>())))
    {
        using std::swap;
        auto& old_obj = *get();
        auto& old_rhs_obj = *rhs.get();
        storage.swap_object(rhs.storage);
        // If both are inline allocated use 3 way move
        if (storage.size() <= storage.max_size() && 
            rhs.storage.size() <= rhs.storage.max_size()) {
            storage_t temp(storage);
            auto& tobj = *CloningPolicy::Move(::std::move(old_obj), temp.get());
            obj = CloningPolicy::Move(::std::move(old_rhs_obj),get());
            rhs.obj = CloningPolicy::Move(::std::move(tobj), rhs.storage.get());
            tobj.~IF();
        }
        // If one was inline allocated move only the inline one
        else if (storage.size() <= storage.max_size() && 
            rhs.storage.size() > rhs.storage.max_size()) {
            swap_w_inline_allocated(*this, std::move(old_rhs_obj),rhs,old_obj);
        }
        else if (rhs.storage.size() <= rhs.storage.max_size() &&
            storage.size() > storage.max_size()) {
            swap_w_inline_allocated(rhs, std::move(old_obj),*this,old_rhs_obj);
        }
        // just swap objects
        else {
            swap(obj, rhs.obj);
        }
    }

    ~polymorphic_obj_storage_t()
    {
        destroy();
    }

    type* get() noexcept
    {
        return obj;
    }

    const type* get() const noexcept
    {
        return obj;
    }

    operator bool()const noexcept
    {
        return obj != nullptr;
    }

    type* operator->() noexcept
    {
        return obj;
    }

    const type* operator->() const noexcept
    {
        return obj;
    }

    allocator_type& get_allocator() noexcept
    {
        return storage.get_allocator();
    }

    const allocator_type& get_allocator() const noexcept
    {
        return storage.get_allocator();
    }

private:
    static void swap_w_inline_allocated(polymorphic_obj_storage_t& dest, IF&& old_obj, 
        polymorphic_obj_storage_t& src, IF& swapped_obj) noexcept
    {
        dest.obj = CloningPolicy::Move(::std::move(old_obj), dest.storage.get());
        old_obj.~IF();
        src.obj = &swapped_obj;
    }

    // precondition: object has been cleaned up, rhs has an active,
    // non-inline allocated object
    void move_assign_w_allocated_obj(polymorphic_obj_storage_t&& rhs,
        ::std::true_type) noexcept
    {
        using std::swap;
        storage = ::std::move(rhs.storage);
        swap(obj, rhs.obj);
    }

    // precondition: object has been cleaned up, rhs has an active,
    // non-inline allocated object
    void move_assign_w_allocated_obj(polymorphic_obj_storage_t&& rhs,
        ::std::false_type)
    {
        using std::swap;
        // store old storage for comparison
        auto old_storage = rhs.storage.get();
        storage = ::std::move(rhs.storage);
        // if no reallocation happened, swap obj pointers
        if (old_storage == storage.get()) {
            swap(obj, rhs.obj);
        }
        // else move object to newly allocated storage
        else {
            obj = CloningPolicy::Move(::std::move(*rhs.get()),
                storage.get());
        }
    }

    void destroy() noexcept
    {
        if (obj) {
            obj->~IF();
        }
        obj = nullptr;
    }

    void cleanup() noexcept
    {
        destroy();
        storage.deallocate();
    }

    //////////////////////////
    ///// member variables
    /////////////////////////
    storage_t storage;
    type* obj;
};

template<typename IF>
using polymorphic_obj_storage = polymorphic_obj_storage_t<IF>;


template<size_t s,size_t a, class A>
inline void swap(sso_storage_t<s,a,A>& lhs ,sso_storage_t<s,a,A>& rhs)
noexcept(noexcept(std::declval<::estd::sso_storage_t<s, a, A>&>()
        .swap_object(std::declval<::estd::sso_storage_t<s, a, A>&>())))
{
    lhs.swap_object(rhs);
}

template<class I,class C,size_t s, size_t a, class A>
inline void swap(polymorphic_obj_storage_t<I,C,s, a, A>& lhs, polymorphic_obj_storage_t<I,C,s, a, A>& rhs)
noexcept(noexcept(std::declval<::estd::polymorphic_obj_storage_t<I, C, s, a, A>&>()
    .swap_object(std::declval<::estd::polymorphic_obj_storage_t<I, C, s, a, A>&>())))
{
    lhs.swap_object(rhs);
}


class memory_resource_t {
public:
    memory_resource_t() noexcept : memory{} {}
    memory_resource_t(void* p,size_t n) : memory{p,n} {
        if (!p || !n)throw std::logic_error("invalid memory resource");
    }
    memory_resource_t(const memory_resource_t&) noexcept = default;
    memory_resource_t(memory_resource_t&&) noexcept = default;
    memory_resource_t& operator=(const memory_resource_t& rhs) noexcept {
        memory = rhs.memory;
        return *this;
    }
    memory_resource_t& operator=(memory_resource_t&&) noexcept = default;

    operator void*() const noexcept {
        return memory.first;
    }
    operator bool()const noexcept {
        return memory.first != nullptr;
    }
    void* ptr()const noexcept {
        return memory.first;
    }
    size_t size() const noexcept {
        return memory.second;
    }
private:
    std::pair<void*, size_t> memory;
};

struct poly_alloc_t {
    virtual void* allocate(size_t n, const void* hint = nullptr) = 0;
    virtual void deallocate(void* p, size_t n) noexcept = 0;
    virtual size_t max_size() const noexcept = 0;
    virtual poly_alloc_t* clone(poly_alloc_t& a) const = 0;
    virtual bool operator==(const poly_alloc_t&) const noexcept = 0;
    bool operator!=(const poly_alloc_t& rhs) const noexcept {
        return !(*this == rhs);
    }
    virtual ~poly_alloc_t() = default;
};


template<typename T>
class poly_alloc_wrapper;

template<class Alloc>
class poly_alloc_impl : public poly_alloc_t, private Alloc {
public:
    using allocator_traits = std::allocator_traits<Alloc>;
    using allocator_type = typename allocator_traits::allocator_type;
    using value_type = typename allocator_traits::value_type;
    using pointer = typename allocator_traits::pointer;
    using const_pointer = typename allocator_traits::const_pointer;

    static_assert(std::is_same<uint8_t, value_type>::value, "Alloc has to be a byte allocator");

    void* allocate(size_t n, const void* hint = nullptr) override {
        return this->allocator_type::allocate(n,static_cast<const_pointer>(hint));
    }

    void deallocate(void* p, size_t n) noexcept override
    {
        this->allocator_type::deallocate(static_cast<pointer>(p), n);
    }
    
    size_t max_size() const noexcept override 
    {
        return this->allocator_type::max_size();
    }
    
    bool operator==(const poly_alloc_t& rhs) const noexcept override {
        auto other = dynamic_cast<const poly_alloc_impl*>(&rhs);
        return  other != nullptr && *this == *other;
    }

    allocator_type& allocator() noexcept {
        return static_cast<allocator_type&>(*this);
    }

    const allocator_type& allocator() const noexcept {
        return static_cast<const allocator_type&>(*this);
    }


    poly_alloc_t* clone(poly_alloc_t& a) const override 
    {
        poly_alloc_wrapper<poly_alloc_impl> allocator(a);
        auto deleter = [&](poly_alloc_impl* p) { allocator.deallocate(p, 1); };
        std::unique_ptr<poly_alloc_impl, std::function<void(poly_alloc_impl*)>> 
            p{ allocator.allocate(1),std::move(deleter)};
        new(p.get()) poly_alloc_impl(*this);
        return p.release();
    }

};

template<typename T>
class poly_alloc_wrapper {
public:
    using pointer = T*;
    using const_pointer = const T*;
    using value_type = T;
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using is_always_equal = std::false_type;

    template<typename TT>
    poly_alloc_wrapper(poly_alloc_wrapper<TT>& p) noexcept : _a{ p.allocator() } {}

    poly_alloc_wrapper(poly_alloc_t& p) noexcept : _a{ p } {}

    template<typename TT>
    poly_alloc_wrapper& operator=(poly_alloc_wrapper<TT>& p) noexcept
    {
        _a = p.allocator();
        return *this;
    }

    poly_alloc_wrapper select_on_container_copy_construction() noexcept {
        return *this;
    }

    pointer allocate(size_t n) {
        return static_cast<pointer>(_a.get().allocate(n * sizeof(T)));
    }
    
    void deallocate(pointer p, size_t n) noexcept {
        _a.get().deallocate(p, n * sizeof(T));
    }

    poly_alloc_t& allocator() noexcept {
        return _a;
    }
    const poly_alloc_t& allocator() const noexcept {
        return _a;
    }

    template<typename TT>
    bool operator==(const poly_alloc_wrapper<TT>& rhs)const noexcept {
        return allocator() == rhs.allocator();
    }

    template<typename TT>
    bool operator!=(const poly_alloc_wrapper<TT>& rhs)const noexcept {
        return !(*this == rhs);
    }

    size_t max_size() const noexcept
    {
        return allocator().max_size();
    }

private:
    std::reference_wrapper<poly_alloc_t> _a;
};


}  //namespace estd


#endif  /* MEMORY_H_ */
