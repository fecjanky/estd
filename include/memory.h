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
    template<size_t P,size_t N>
    struct is_power_of;

    template<size_t P,size_t N>
    struct is_power_of {
        static constexpr size_t remainder = N % P;
        static_assert( N == 1 ? true : remainder == 0, "N is not a power of P");
        static constexpr bool value = is_power_of<P,N/P>::value;
    };

    template<size_t P>
    struct is_power_of<P,1> {
        static constexpr bool value = true;
    };

    template<size_t P>
    struct is_power_of<P,0> {
        static constexpr bool value = false;
    };

    struct DefaultCloningPolicy {
        template<typename T>
        static T* Clone(const T* from, void* to) {
            return from->clone(to);
        }
    };

}

template<
    size_t max_size_in_ptr_size = 4,
    size_t align = alignof(std::max_align_t),
    class Allocator = std::allocator<uint8_t>
>
class obj_storage_t : private std::allocator_traits<Allocator>::template rebind_alloc<uint8_t> {
public:
    static constexpr size_t max_size =
            max_size_in_ptr_size < 1 ?
                    sizeof(void*) : max_size_in_ptr_size * sizeof(void*);
    static constexpr size_t alignment = align;
    static_assert(impl::is_power_of<2,alignment>::value,"alignment is not a power of 2");

    using MyAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<uint8_t>;

    obj_storage_t():storage{},size{}{}

    explicit obj_storage_t(size_t n):storage{},size{}{
        allocate(n);
    }

    obj_storage_t(const obj_storage_t& rhs) : storage{},size {} {
        if (rhs.size && rhs.size <= max_size)
            allocate_inline(rhs.size);
        else if(rhs.size)
            allocate_on_heap(rhs.size);
    }

    obj_storage_t& operator= (const obj_storage_t& rhs) {
        deallocate();
        if (rhs.size > 0) allocate(rhs.size);
    }

    void* allocate(size_t n){
        n = n == 0 ? 1 : n;
        allocation_check();
        if(n <= max_size)
            return allocate_inline(n);
        else
            return allocate_on_heap(n);
    }

    void deallocate(){
        if(size > max_size)
            this->MyAllocator::deallocate(heap_storage,size);
        size = 0;
    }

    ~obj_storage_t(){
        deallocate();
    }

    operator bool ()const{
        return size != 0;
    }

    void* get() const {
        if(size == 0)
            throw std::runtime_error("accessing unallocated storage");
        else if(size>max_size)
            return aligned_heap_addr(heap_storage);
        else
            return &*storage.begin();
    }

    template<size_t N>
    static constexpr bool is_alignment_ok() {
        return N <= alignment && impl::is_power_of<2, N>::value;
    }

private:

    void* allocate_inline(size_t n){
        size = n;
        return &*storage.begin();
    }

    void* allocate_on_heap(size_t n){
        // allocating +alignment bytes to be able to
        // align heap storage as well
        heap_storage = static_cast<uint8_t*>(this->allocate(n+alignment));
        return aligned_heap_addr(heap_storage);
    }

    void allocation_check(){
        if(size != 0)
            throw std::bad_alloc{};
    }

    static void* aligned_heap_addr(uint8_t* ptr){
            return ptr + reinterpret_cast<uintptr_t>(ptr) % alignment;
    }

    ///////////////

     union alignas(alignment) {
         mutable std::array<std::uint8_t,max_size> storage;
         mutable std::uint8_t* heap_storage;
    };

    size_t size;
};

using obj_storage = obj_storage_t<>;

template<
    class IF,
    typename CloningPolicy = impl::DefaultCloningPolicy,
    size_t max_size_in_ptr_size = 4,
    size_t align = alignof(std::max_align_t),
class Allocator = std::allocator<uint8_t>
>
class polymorphic_obj_storage_t {
public:
    static_assert(std::is_polymorphic<IF>::value, "IF class is not polymorphic");

    template<
        typename T, 
        typename = std::enable_if_t < std::is_base_of<IF,std::decay_t<T>>::value >
    > polymorphic_obj_storage_t(T&& t) : 
        storage{sizeof(T)} ,
        obj{ new (static_cast<std::decay_t<T>*>(storage.get())) 
                std::decay_t<T>(std::forward<T>(t)) } 
    {
        static_assert(
            storage_t::is_alignment_ok<alignof(std::decay_t<T>)>(),
            "T is not properly aligned");
    }

    polymorphic_obj_storage_t() : storage{}, obj{} 
    {
    }

    polymorphic_obj_storage_t(const polymorphic_obj_storage_t& rhs) : 
        storage{rhs.storage},
        obj { CloningPolicy::Clone(rhs.get(), storage.get())} 
    {
    }

    polymorphic_obj_storage_t& operator=(const polymorphic_obj_storage_t& rhs) 
    {
        if (this != &rhs) {
            destroy();
            storage = rhs.storage;
            obj = CloningPolicy::Clone(rhs.get(),storage.get());
        }
        return *this;
    }


    ~polymorphic_obj_storage_t() {
        destroy();
    }


    IF* get() {
        return obj;
    }

    const IF* get() const {
        return obj;
    }

    operator bool()const
    {
        return obj != nullptr;
    }

    IF* operator->() {
        return obj;
    }

    const IF* operator->() const {
        return obj;
    }

private:
    void destroy() {
        if (obj)
            obj->~IF();
        obj = nullptr;
    }

    using storage_t = obj_storage_t<
        max_size_in_ptr_size < 2 ? 1 : max_size_in_ptr_size - 1,
        align,
        Allocator
    >;

    storage_t storage;
    IF* obj;
};

template<typename IF>
using polymorphic_obj_storage = polymorphic_obj_storage_t<IF>;

} //namespace estd



#endif /* MEMORY_H_ */
