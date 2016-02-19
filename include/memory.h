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

struct DefaultCloningPolicy {
    template<typename T>
    static T* Clone(const T& from, void* to)
    {
        return from.clone(to);
    }

    template<typename T, typename = std::enable_if_t<
            !std::is_lvalue_reference<T>::value>>
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

}  //namespace impl

template<
    size_t storage_size = 4, // in pointer size
    size_t alignment = alignof(std::max_align_t),
    template<typename > class Allocator = std::allocator
>
class obj_storage_t: private Allocator<uint8_t> {
public:
    static constexpr size_t max_size_ =
            storage_size < 1 ? sizeof(void*) : storage_size * sizeof(void*);
    static_assert(impl::is_power_of<2,alignment>::value,"alignment is not a power of 2");

    using allocator_type = Allocator<uint8_t>;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using pocma = typename allocator_traits::propagate_on_container_move_assignment;
    using pocca = typename allocator_traits::propagate_on_container_copy_assignment;
    using pointer = typename allocator_traits::pointer;

    obj_storage_t() :
            storage { }, size_ { }
    {
    }

    template<typename A, typename = std::enable_if_t<
            !std::is_base_of<obj_storage_t, std::decay_t<A>>::value
                    && std::is_constructible<allocator_type, std::add_lvalue_reference_t<A>>::value> 
    > obj_storage_t( A&& a) :
            Allocator<uint8_t>(std::forward<A>(a)), storage { }, size_ { }
    {
    }
    
    explicit obj_storage_t(size_t n) :
            storage { }, size_ { }
    {
        allocate(n);
    }

    obj_storage_t(const obj_storage_t& rhs) :
            Allocator<uint8_t>(
                allocator_traits::select_on_container_copy_construction(
                            rhs.get_allocator())), storage { }, size_ { }
    {
        if (rhs.size_ > 0)
            allocate(rhs.size_);
    }

    obj_storage_t(obj_storage_t&& rhs) noexcept
    : Allocator<uint8_t>(std::move(rhs)), heap_storage {}, size_ {}
    {
        static_assert(std::is_nothrow_move_constructible<allocator_type>::value,
            "allocator_type is not nothrow_move_constructible");
        obtain_rvalue(std::move(rhs));
    }

    obj_storage_t& operator= (const obj_storage_t& rhs)
    {
        if (this != &rhs) {
            // make allocator based allocation first to
            // provide strong exception guarantee
            pointer p{};
            if (rhs.size() > rhs.max_size()) {
                if (pocca::value) {
                    p = rhs.get_allocator().allocate(rhs.size());
                } else {
                    p = get_allocator().allocate(rhs.size());
                }
            }
            // deallocate with old allocator
            deallocate();
            // copy rhs allocator if required
            if (pocca::value) {
                static_assert(std::is_nothrow_copy_assignable<allocator_type>::value,
                    "allocator_type is not nothrow_copy_assignable");
                static_cast<allocator_type>(*this) = rhs;
            }
            assign_storage(p, rhs);
        }
        return *this;
    }

    obj_storage_t& operator= (obj_storage_t&& rhs) noexcept(pocma::value)
    {
        if (this != &rhs) {
            if (pocma::value) {
                deallocate();
                obtain_rvalue(std::move(rhs));
                static_assert(std::is_nothrow_move_assignable<allocator_type>::value,
                    "allocator_type is not nothrow_move_assignable");
                static_cast<allocator_type>(*this) = std::move(rhs);
            }
            else {
                // can obtain storage only if allocators compare to equal
                if (static_cast<allocator_type>(*this) == rhs) {
                    deallocate();
                    obtain_rvalue(std::move(rhs));
                }
                // need to reallocate with this allocator if not equal
                else {
                    // make allocation first to provide strong guarantee
                    pointer p{};
                    if (rhs.size() > rhs.max_size()) {
                        p = get_allocator().allocate(rhs.size());
                    }
                    deallocate();
                    assign_storage(p, rhs);
                }
            }
        }
        return *this;
    }

    void* allocate(size_t n)
    {
        allocation_check();
        n = n == 0 ? 1 : n;
        if(n <= max_size_) {
            return allocate_inline(n);
        } else {
            return allocate_with_allocator(n);
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
            throw std::runtime_error("accessing unallocated storage");
        }
        return get();
    }

    void* get() noexcept
    {
        if (size_>max_size_) {
            return aligned_heap_addr(heap_storage);
        } else {
            return &*storage.begin();
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

    void* allocate_inline(size_t n) noexcept
    {
        size_ = n;
        return &*storage.begin();
    }

    void* allocate_with_allocator(size_t n)
    {
        // allocating +alignment bytes to be able to
        // align heap storage as well
        auto alloc_size = n + alignment;
        heap_storage = static_cast<uint8_t*>(this->allocator_type::allocate(alloc_size));
        size_ = alloc_size;
        return aligned_heap_addr(heap_storage);
    }

    void allocation_check()
    {
        if (size_ != 0) {
            throw std::bad_alloc {};
        }
    }

    static void* aligned_heap_addr(uint8_t* ptr) noexcept
    {
        return ptr + reinterpret_cast<uintptr_t>(ptr) % alignment;
    }

    // precondition: this storage is in deallocated state
    void obtain_rvalue(obj_storage_t&& rhs) noexcept
    {
        // leave rhs as is if inline allocation happens
        if (rhs.size() > 0 && rhs.size() <= max_size_) {
            allocate_inline(rhs.size());
        }
        else if (rhs.size()) {
            std::swap(size_, rhs.size_);
            std::swap(heap_storage, rhs.heap_storage);
        }
    }

    // precondition: this storage is in deallocated state
    void assign_storage(pointer p,const obj_storage_t& rhs) noexcept {
        // use preallocated storage 
        if (rhs.size() > rhs.max_size()) {
            heap_storage = p;
            size_ = rhs.size();
            // allocate inline else
        }
        else if (rhs.size() > 0) {
            allocate_inline(rhs.size());
        }
    }

    ///////////////
    // Data members
    ///////////////

    union alignas(alignment) {
        std::array<std::uint8_t,max_size_> storage;
        std::uint8_t* heap_storage;
    };

    size_t size_;
};

using obj_storage = obj_storage_t<>;

// TODO(fecjanky): specialize std::swap for obj_storage_t

template<size_t s, size_t a, template<typename > class A>
bool operator==(const obj_storage_t<s, a, A>& lhs,
        const obj_storage_t<s, a, A>& rhs) noexcept
{
    return (!lhs && !rhs) || ((lhs && rhs) && (lhs.get() == rhs.get()));
}

template<size_t s, size_t a, template<typename > class A>
bool operator!=(const obj_storage_t<s, a, A>& lhs,
        const obj_storage_t<s, a, A>& rhs) noexcept
{
    return !(lhs == rhs);
}

template<
    class IF,
    typename CloningPolicy = impl::DefaultCloningPolicy,
    size_t storage_size = 4,  // in pointer size
    size_t alignment = alignof(std::max_align_t),
    template<typename > class Allocator = std::allocator
>
class polymorphic_obj_storage_t {
public:
    using storage_t = obj_storage_t<storage_size, alignment,Allocator>;

    using allocator_type = typename storage_t::allocator_type;
    using allocator_traits = std::allocator_traits<allocator_type>;
    using pocma = typename allocator_traits::propagate_on_container_move_assignment;

    static_assert(std::is_polymorphic<IF>::value, "IF class is not polymorphic");

    template<
        typename T,
        typename = std::enable_if_t<std::is_base_of<IF, std::decay_t<T>>::value>
    >
    polymorphic_obj_storage_t(T&& t) :
        storage { sizeof(T) },
        obj { new (reinterpret_cast<std::decay_t<T>*>(storage.get()))
            std::decay_t<T>(std::forward<T>(t)) }
    {
        static_assert( storage_t::is_alignment_ok(
                        impl::alignment_t<alignof(std::decay_t<T>)> {}),
                "T is not properly aligned");
    }

    polymorphic_obj_storage_t() :
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
    : storage {std::move(rhs.storage)}, obj {rhs.obj}
    {
        // successful storage swap,
        if (storage.size() > storage.max_size()) {
            rhs.obj = nullptr;
        }
        // need to move object if inline allocation happened
        // and rhs has an object
        else if (rhs) {
            obj = CloningPolicy::Move(std::move(*rhs.get()), storage.get());
        }
    }

    polymorphic_obj_storage_t& operator=(const polymorphic_obj_storage_t& rhs)
    {
        if (this != &rhs) {
            destroy();
            storage = rhs.storage;
            obj = rhs.get() ? CloningPolicy::Clone(*rhs.get(),storage.get()) : nullptr;
        }
        return *this;
    }

    polymorphic_obj_storage_t& operator=(polymorphic_obj_storage_t&& rhs)
        noexcept(pocma::value)
    {
        if (this != &rhs) {
            destroy();
            // if no object in rhs, nothing to do
            if (rhs) {
                // for inline object inline allocation has to be made
                if (rhs.storage.size() <= rhs.storage.max_size()) {
                    storage.allocate(rhs.storage.size());
                    obj = CloningPolicy::Move(std::move(*rhs.get()),
                            storage.get());
                }
                else {
                    // store old storage for comparison
                    auto old_storage = rhs.storage.get();
                    storage = std::move(rhs.storage);
                    // if no reallocation happened, swap obj pointers
                    if (old_storage == storage.get()) {
                        obj = rhs.obj;
                        rhs.obj = nullptr;
                    }
                    // else move object to newly allocated storage
                    else {
                        obj = CloningPolicy::Move(std::move(*rhs.get()), storage.get());
                    }
                }
            }
        }
        return *this;
    }

    ~polymorphic_obj_storage_t()
    {
        destroy();
    }

    IF* get()
    {
        return obj;
    }

    const IF* get() const
    {
        return obj;
    }

    operator bool()const
    {
        return obj != nullptr;
    }

    IF* operator->()
    {
        return obj;
    }

    const IF* operator->() const
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
    void destroy() noexcept
    {
        if (obj)
        obj->~IF();
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
    IF* obj;
};

template<typename IF>
using polymorphic_obj_storage = polymorphic_obj_storage_t<IF>;

}  //namespace estd

#endif  /* MEMORY_H_ */
