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
struct AndT;

template<typename... Ts>
struct AndT<::std::true_type, Ts...> {
    using value = typename AndT<Ts...>::value;
};

template<typename... Ts>
struct AndT<::std::false_type , Ts...> {
    using value = ::std::false_type;
};

template<>
struct AndT<void> {
    using value = ::std::true_type;
};


struct DefaultCloningPolicy {
    template<typename T>
    static T* Clone(const T& from, void* to)
    {
        return from.clone(to);
    }

    template<typename T, typename = ::std::enable_if_t<
            !::std::is_lvalue_reference<T>::value>>
    static T* Move(T&& from, void* to)
    {
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
class obj_storage_t: private Allocator {
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

    obj_storage_t() :
            storage { }, heap_storage{}, size_ { }
    {
    }

    template<typename A> 
    obj_storage_t(::std::allocator_arg_t, A&& a) :
        Allocator(::std::forward<A>(a)), storage { }, heap_storage{}, size_ { }
    {
    }
    
    explicit obj_storage_t(size_t n) :
            storage { }, heap_storage{},size_ { }
    {
        allocate(n);
    }

    obj_storage_t(const obj_storage_t& rhs) :
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

    obj_storage_t(obj_storage_t&& rhs) noexcept
    : Allocator(::std::move(rhs)), storage{}, heap_storage {}, size_ {}
    {
        static_assert(::std::is_nothrow_move_constructible<allocator_type>::value,
            "allocator_type is not nothrow_move_constructible");
        obtain_rvalue(::std::move(rhs));
    }

    obj_storage_t& operator= (const obj_storage_t& rhs)
    {
        return copy_assign_impl(rhs, pocca{});
    }

    obj_storage_t& operator= (obj_storage_t&& rhs) noexcept(pocma::value)
    {
        return move_assign_impl(::std::move(rhs), pocma{});
    }

    void swap(obj_storage_t& rhs) noexcept(pocs::value)
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

    ~obj_storage_t()
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

    obj_storage_t& copy_assign_impl(const obj_storage_t& rhs, ::std::true_type)
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
            assign_storage(p, rhs);
        }
        return *this;
    }

    obj_storage_t& copy_assign_impl(const obj_storage_t& rhs, ::std::false_type)
    {
        if (this != &rhs) {
            strong_copy_allocation(std::move(rhs));
        }
        return *this;
    }


    obj_storage_t& move_assign_impl(obj_storage_t&& rhs, ::std::true_type) noexcept
    {
        static_assert(::std::is_nothrow_move_assignable<allocator_type>::value,
            "allocator_type is not nothrow_move_assignable");

        if (this != &rhs) {
            deallocate();
            obtain_rvalue(::std::move(rhs));
            get_allocator() = ::std::move(rhs.get_allocator());
        }
        return *this;
    }

    obj_storage_t& move_assign_impl(obj_storage_t&& rhs, ::std::false_type)
    {
        if (this != &rhs) {
            // can obtain storage only if allocators compare to equal
            if (get_allocator() == rhs.get_allocator()) {
                deallocate();
                obtain_rvalue(::std::move(rhs));
            }
            // need to reallocate with this allocator if not equal
            else {
                strong_copy_allocation(rhs);
            }
        }
        return *this;
    }

    void strong_copy_allocation(const obj_storage_t& rhs)
    {
        // make allocator based allocation first to
        // provide strong exception guarantee
        pointer p{};
        if (rhs.size() > rhs.max_size()) {
            p = get_allocator().allocate(rhs.size());
        }
        deallocate();
        assign_storage(p, rhs);
    }

    void swap_impl(obj_storage_t& rhs, ::std::true_type) noexcept
    {
        ::std::swap(get_allocator(),rhs.get_allocator());
        swap_guts(rhs);
    }

    void swap_impl(obj_storage_t& rhs, ::std::false_type)
    {
        if(!(get_allocator() == rhs.get_allocator())) {
            throw ::std::runtime_error(
                    "obj_storage_t: swap attempt with unequal allocators ");
        }
        swap_guts(rhs);
    }

    void swap_guts(obj_storage_t& rhs) noexcept{
        ::std::swap(heap_storage,rhs.heap_storage);
        ::std::swap(size_,rhs.size_);
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
    void obtain_rvalue(obj_storage_t&& rhs) noexcept
    {
        // leave rhs as is if inline allocation happens
        if (rhs.size() > 0 && rhs.size() <= max_size_) {
            allocate_inline(rhs.size());
        }
        else if (rhs.size() > 0) {
            swap_guts(::std::move(rhs));
        }
    }

    // precondition: this storage is in deallocated state
    void assign_storage(pointer p,const obj_storage_t& rhs) noexcept 
    {
        // use preallocated storage 
        if (rhs.size() > 0 && rhs.size() <= max_size_) {
            allocate_inline(rhs.size());
        }
        // use allocated storage else
        else if (rhs.size() > 0) {
            heap_storage = p;
            size_ = rhs.size();
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

using obj_storage = obj_storage_t<>;

template<size_t s, size_t a, class A>
bool operator==(const obj_storage_t<s, a, A>& lhs,
        const obj_storage_t<s, a, A>& rhs) noexcept
{
    return (!lhs && !rhs) || ((lhs && rhs) && (lhs.get() == rhs.get()));
}

template<size_t s, size_t a, class A>
bool operator!=(const obj_storage_t<s, a, A>& lhs,
        const obj_storage_t<s, a, A>& rhs) noexcept
{
    return !(lhs == rhs);
}

template<
    class IF,
    typename CloningPolicy = impl::DefaultCloningPolicy,
    size_t storage_size = 4,  // in pointer size
    size_t alignment = alignof(::std::max_align_t),
    template<typename > class Allocator = ::std::allocator
>
class polymorphic_obj_storage_t {
public:
    using storage_t = obj_storage_t<storage_size, alignment,Allocator<uint8_t>>;

    using allocator_type = typename storage_t::allocator_type;
    using allocator_traits = ::std::allocator_traits<allocator_type>;
    using type = IF;
    static_assert(::std::is_polymorphic<type>::value, "IF class is not polymorphic");
    static_assert(::std::is_same<type, ::std::decay_t<type>>::value, "IF class must be cv unqualifed and non-reference type");
    static_assert(noexcept(CloningPolicy::Move(::std::move(::std::declval<type>()), ::std::declval<void*>())),
        "IF type is not noexcept moveable");

    template<
        typename T,
        typename = ::std::enable_if_t<::std::is_base_of<type, ::std::decay_t<T>>::value>
    >
    polymorphic_obj_storage_t(T&& t) :
        storage { sizeof(::std::decay_t<T>) },
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
            destroy();
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
        if (!rhs) {
            destroy();
            storage = ::std::move(rhs.storage);
            return *this;
        }

        if (this != &rhs) {
            // inline allocation case
            if (rhs.storage.size() <= rhs.storage.max_size()) {
                destroy();
                storage.allocate(rhs.storage.size());
                obj = CloningPolicy::Move(::std::move(*rhs.get()),
                        storage.get());
            } else {
                move_assign_w_allocated_obj(::std::move(rhs),
                    ::std::is_nothrow_move_assignable<storage_t>{});
            }
        }
        return *this;
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

    void move_assign_w_allocated_obj(polymorphic_obj_storage_t&& rhs,
        ::std::true_type) noexcept
    {
        destroy();
        storage = ::std::move(rhs.storage);
        ::std::swap(obj, rhs.obj);
    }

    void move_assign_w_allocated_obj(polymorphic_obj_storage_t&& rhs,
        ::std::false_type)
    {
        destroy();
        // store old storage for comparison
        auto old_storage = rhs.storage.get();
        storage = ::std::move(rhs.storage);
        // if no reallocation happened, swap obj pointers
        if (old_storage == storage.get()) {
            ::std::swap(obj, rhs.obj);
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

}  //namespace estd

namespace std{

template<size_t s,size_t a, class A>
inline void swap(::estd::obj_storage_t<s,a,A>& lhs,::estd::obj_storage_t<s,a,A>& rhs)
noexcept(noexcept(std::declval<::estd::obj_storage_t<s, a, A>>().swap(std::declval<::estd::obj_storage_t<s, a, A>>())))
{
    lhs.swap(::std::move(rhs));
}

}  // namespace std


#endif  /* MEMORY_H_ */
