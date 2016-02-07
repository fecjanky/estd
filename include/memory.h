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

}

template<
    size_t max_size_in_ptr_size = 4,
    size_t align = alignof(double),
    class Allocator = std::allocator<uint8_t>
>
class obj_storage_t : private std::allocator_traits<Allocator>::template rebind_alloc<uint8_t> {
public:
    static constexpr size_t max_size =
            max_size_in_ptr_size < 1 ?
                    sizeof(void*) : max_size_in_ptr_size * sizeof(void*);
    static constexpr size_t alignment = align;
    static_assert(impl::is_power_of<2,alignment>::value,"alignment is not a power of 2");

    obj_storage_t():storage{},size{}{}

    explicit obj_storage_t(size_t n):storage{},size{}{
        allocate(n);
    }

    void* allocate(size_t n){
        n = n == 0 ? 1 : n;
        allocation_check();
        if(n < max_size)
            return allocate_inline(n);
        else
            return allocate_on_heap(n);
    }

    void deallocate(){
        if(size > max_size)
            delete[] heap_storage;
        size = 0;
    }

    ~obj_storage_t(){
        deallocate();
    }

    void* get() const {
        if(size == 0)
            throw std::runtime_error("accessing unallocated storage");
        else if(size>max_size)
            return aligned_heap_addr(heap_storage);
        else
            return &*storage.begin();
    }

private:
    void* allocate_inline(size_t n){
        size = n;
        return &*storage.begin();
    }

    void* allocate_on_heap(size_t n){
        // allocating +alignment bytes to be able to
        // align heap storage as well
        heap_storage = new uint8_t[n+alignment];
        return aligned_heap_addr(heap_storage);
    }

    void allocation_check(){
        if(size != 0)
            throw std::bad_alloc{};
    }

    static void* aligned_heap_addr(uint8_t* ptr){
        if(reinterpret_cast<uintptr_t>(ptr) % alignment)
            return ptr + reinterpret_cast<uintptr_t>(ptr) % alignment;
        else
            return ptr;
    }

    ///////////////

     union alignas(alignment) {
        std::array<std::uint8_t,max_size> storage;
        std::uint8_t* heap_storage;
    };

    size_t size;
};

using obj_storage = obj_storage_t<>;

} //namespace estd



#endif /* MEMORY_H_ */
