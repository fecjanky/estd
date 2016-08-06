#ifndef POLY_VECTOR_H_
#define POLY_VECTOR_H_

#include <memory>
#include <cstdint>
#include <utility>
#include <tuple>
#include <algorithm>
#include <cassert>
#include "memory.h"

namespace estd {
    template<class Allocator = std::allocator<uint8_t> >
    struct allocator_base : private Allocator {
        
        using allocator_traits = std::allocator_traits<Allocator>;
        using value_type = typename allocator_traits::value_type;
        using propagate_on_container_copy_assignment = typename allocator_traits::propagate_on_container_copy_assignment;
        static_assert(std::is_same<value_type, uint8_t>::value, "requires a byte allocator");
        using allocator_type = Allocator;
        using void_ptr = void*;
        //////////////////////////////////////////////
        allocator_base(const Allocator& a = Allocator()) : Allocator(a), _storage{}, _end_storage{} {}
        allocator_base(size_t n,const Allocator& a = Allocator()) : Allocator(a), _storage{}, _end_storage{} 
        {
            auto s = get_allocator().allocate(n);
            _storage = s;
            _end_storage = s + n;
        }

        allocator_base(const allocator_base& a) :
            allocator_base(allocator_traits::select_on_container_copy_construction(a.get_allocator()))
        {
            if (a.size())allocate(a.size());
        }
        allocator_base& operator=(allocator_base& a)
        {
            return copy_assign_impl(a, propagate_on_container_copy_assignment{});
        }
        allocator_base(allocator_base&& a) noexcept :
            Allocator(std::move(a.get_allocator())), _storage{}, _end_storage{}
        {
            using std::swap;
            swap(a._storage, _storage);
            swap(a._end_storage, _end_storage);
        }
        allocator_base& operator=(allocator_base&& a) noexcept
        {
            // TODO: check for allocator traits
            using std::swap;
            swap(a._storage, _storage);
            swap(a._end_storage, _end_storage);
            return *this;
        }


        allocator_base& copy_assign_impl(const allocator_base& a,std::true_type)
        {
            // Note: allocator copy assignment must not throw
            auto s = a.get_allocator().allocate(n);
            tidy();
            get_allocator() = a.get_allocator();
            _storage = s;
            _end_storage = s + n;
            return *this;
        }
        allocator_base& copy_assign_impl(const allocator_base& a, std::false_type)
        {
            auto s = get_allocator().allocate(n);
            tidy();
            _storage = s;
            _end_storage = s + n;
            return *this;
        }

        void allocate(size_t n)
        {
            auto s = get_allocator().allocate(n);
            tidy();
            _storage = s;
            _end_storage = s + n;
        }

        allocator_type& get_allocator()noexcept
        {
            return *this;
        }
        void tidy() noexcept
        {
            get_allocator().deallocate(reinterpret_cast<uint8_t*>(_storage), size());
            _storage = _end_storage = nullptr;
        }
        ~allocator_base()
        {
            tidy();
        }

        size_t size()const noexcept {
            return reinterpret_cast<uint8_t*>(_end_storage) -
                reinterpret_cast<uint8_t*>(_storage);
        }
        ////////////////////////////////
        void* _storage;
        void* _end_storage;
    };

    template<class IF>
    struct DefaultCloningPolicy {
        static IF* Clone(IF* obj, void* dest) {
            return obj->clone(dest);
        }
    };

    template<class IF>
    class poly_vector_iterator{
    public:
        using void_ptr = void*;
        using interface_type = IF;
        using interface_ptr = interface_type*;
        using elem_ptr = std::pair<void_ptr, interface_ptr>;

        poly_vector_iterator() : curr{} {}
        explicit poly_vector_iterator(elem_ptr* p) : curr{ p } {}
        poly_vector_iterator(const poly_vector_iterator&) = default;
        poly_vector_iterator& operator=(const poly_vector_iterator&) = default;
        ~poly_vector_iterator() = default;

        IF* operator->() noexcept{
            return curr->second;
        }
        IF& operator*()noexcept {
            return *curr->second;
        }
        const IF* operator->() const noexcept {
            return curr->second;
        }
        const IF& operator*()const noexcept {
            return *curr->second;
        }

        poly_vector_iterator& operator++()noexcept {
            ++curr;
            return *this;
        }
        poly_vector_iterator operator++(int)noexcept {
            poly_vector_iterator i(*this);
            ++curr;
            return i;
        }
        poly_vector_iterator& operator--()noexcept {
            --curr;
            return *this;
        }
        poly_vector_iterator operator--(int)noexcept {
            poly_vector_iterator i(*this);
            --curr;
            return i;
        }
        bool operator==(const poly_vector_iterator& rhs)const noexcept {
            return curr == rhs.curr;
        }
        bool operator!=(const poly_vector_iterator& rhs)const noexcept {
            return curr != rhs.curr;
        }

    private:
        elem_ptr* curr;
    };

    template<class IF, class Allocator = std::allocator<uint8_t>, class CloningPolicy  = DefaultCloningPolicy<IF> >
    class poly_vector : private allocator_base<Allocator> {
    public:
        using interface_type = IF;
        using interface_ptr = interface_type*;
        using const_interface_ptr = const interface_type*;
        using interface_reference = IF&;
        using const_interface_reference = const IF&;
        using allocator_traits = typename allocator_base::allocator_traits;
        using void_ptr = void*;
        using iterator = poly_vector_iterator<interface_type>;
        using const_iterator = poly_vector_iterator<const interface_type>;
        poly_vector() : _free_elem{}, _begin_storage{}, _free_storage{} {}
        ~poly_vector() 
        {
            tidy();
        }
        template<typename T>
        void push_back(T&& obj)
        {
            using TT = std::decay_t<T>;
            constexpr auto s = sizeof(TT);
            constexpr auto a = alignof(TT);
            if (_free_elem == end_elem()) {
                increase_storage(s, a);
            }
            assert(storage_size(next_aligned_storage(a), _end_storage) >= s);
            _free_elem->first = next_aligned_storage(a);
            _free_elem->second = new (_free_elem->first) TT(std::forward<T>(obj));
            _free_storage = reinterpret_cast<uint8_t*>(_free_elem->first) + s;
            ++_free_elem;
        }

        iterator begin()noexcept {
            return iterator(begin_elem());
        }
        iterator end()noexcept {
            return iterator(end_elem());
        }
        size_t size()const noexcept {
            return _free_elem - begin_elem();
        }

    private:
        void_ptr next_aligned_storage(size_t align) const noexcept{
            auto fs = reinterpret_cast<uintptr_t>(_free_storage);
            return reinterpret_cast<void_ptr>(((fs + align - 1) / align)*align);
        }
        using my_base = allocator_base<Allocator>;
        using elem_ptr = std::pair<void_ptr, interface_ptr>;
        static std::pair<elem_ptr*, void_ptr> poly_uninitialized_copy(
            elem_ptr* begin, elem_ptr* end, 
            std::pair<elem_ptr*, void_ptr> f,
            elem_ptr* dst,size_t n)
        {
            auto o_begin = begin;
            void* dst_storage = dst + n;
            try {
                for (; begin != end; ++begin, ++dst) {
                    dst->first = dst_storage;
                    dst->second = CloningPolicy::Clone(begin->second, dst_storage);
                    dst_storage = next_storage_ptr(begin,f.first,f.second,dst_storage);
                }
                return std::make_pair(dst, dst_storage);
            }
            catch (...) {
                for (; begin != o_begin; --begin) {
                    begin->second->~IF();
                }
                throw;
            }
        }
        void increase_storage(size_t curr_elem_size, size_t align) 
        {
            const auto n = size() ? size() * 2 : 1;
            const auto s = curr_elem_size + align > avg_obj_size() ? curr_elem_size + align : avg_obj_size();
            // allocate temp storage
            my_base a(n*s + n*sizeof(elem_ptr),get_allocator());
            auto ret = poly_uninitialized_copy(
                begin_elem(), end_elem(), std::make_pair(_free_elem, _free_storage),
                reinterpret_cast<elem_ptr*>(a._storage),n);
            tidy();
            my_base::operator=(std::move(a));
            _free_elem = ret.first;
            _free_storage = ret.second;
            _begin_storage = reinterpret_cast<elem_ptr*>(_storage) + n;
        }

        void tidy()
        {
            for (auto src = begin_elem(); src != end_elem(); ++src) {
                src->second->~IF();
            }
            _begin_storage = _free_storage = _free_elem = nullptr;
            my_base::tidy();
        }

        size_t avg_obj_size()const noexcept {
            return storage_size() ? (storage_size() + size()-1) / size() : 0;
        }

        size_t storage_size()const noexcept
        {
            return storage_size(_begin_storage, _free_storage);
        }
        static size_t storage_size(void_ptr b,void_ptr e) noexcept
        {
            return reinterpret_cast<const uint8_t*>(e) -
                reinterpret_cast<const uint8_t*>(b);
        }


        size_t elem_storage_size(const elem_ptr* p)const noexcept
        {
            auto b = reinterpret_cast<const uint8_t*>(p->first);
            ++p;
            return p == end_elem() ? 
                reinterpret_cast<const uint8_t*>(_free_storage) - b :
                reinterpret_cast<const uint8_t*>(p->first) - b;
        }

        static void* next_storage_ptr(elem_ptr* p,elem_ptr* end,void* f,void* dst) noexcept
        {
            auto b = p->first;
            auto e = p + 1 == end ? f : (p + 1)->first;
            return reinterpret_cast<uint8_t*>(dst)+storage_size(b,e);
        }

        elem_ptr* begin_elem()noexcept {
            return reinterpret_cast<elem_ptr*>(_storage);
        }
        elem_ptr* end_elem()noexcept {
            return reinterpret_cast<elem_ptr*>(_begin_storage);
        }
        const elem_ptr* begin_elem()const noexcept {
            return reinterpret_cast<const elem_ptr*>(_storage);
        }
        const elem_ptr* end_elem()const noexcept {
            return reinterpret_cast<const elem_ptr*>(_begin_storage);
        }


        ////////////////////////////
        elem_ptr* _free_elem;
        void* _begin_storage;
        void* _free_storage;
    };

}  // namespace estd

#endif  //POLY_VECTOR_H_
