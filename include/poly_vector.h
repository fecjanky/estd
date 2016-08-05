#ifndef POLY_VECTOR_H_
#define POLY_VECTOR_H_

#include <memory>
#include <cstdint>
#include <utility>
#include <tuple>
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

        template<typename T>
        void push_back(T&& obj)
        {
            using TT = std::decay_t<T>;
            constexpr auto s = sizeof(TT);
            constexpr auto a = alignof(TT);
            if (_free_elem == end_elem()) {
                increase_storage(s, a);
            }
            auto fs = reinterpret_cast<uintptr_t>(_free_storage);
            _free_elem->first = reinterpret_cast<void*>(((fs + a - 1) / a)*a);
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
        using my_base = allocator_base<Allocator>;
        using elem_ptr = std::pair<void_ptr, interface_ptr>;
        // TODO: poly_uninitialized_copy
        // TODO: cleanup of this function
        void increase_storage(size_t curr_elem_size, size_t align) {
            auto s = curr_elem_size + align;
            s = s > avg_obj_size() ? s : avg_obj_size();
            const auto n = size() ? size() * 2 : 1;
            my_base a(n*s + n*sizeof(elem_ptr),get_allocator());
            elem_ptr* dest = reinterpret_cast<elem_ptr*>(a._storage);
            void* dest_storage = dest + n;
            for (auto src = begin_elem(); src != end_elem();++src,++dest) {
                dest->first = dest_storage;
                dest->second = CloningPolicy::Clone(src->second, dest_storage);
                dest_storage = next_storage_ptr(src);
            }
            for (auto src = begin_elem(); src != end_elem(); ++src) {
                src->second->~IF();
            }
            my_base::operator=(std::move(a));
            _free_elem = dest;
            _begin_storage = reinterpret_cast<elem_ptr*>(_storage) + n;
            _free_storage = dest_storage;
        }

        size_t avg_obj_size()const noexcept {
            return storage_size() ? (storage_size() + size()-1) / size() : 0;
        }

        size_t storage_size()const noexcept
        {
            return reinterpret_cast<const uint8_t*>(_free_storage) -
                reinterpret_cast<const uint8_t*>(_begin_storage);
        }

        size_t elem_storage_size(const elem_ptr* p)const noexcept
        {
            auto b = reinterpret_cast<const uint8_t*>(p->first);
            ++p;
            return p == end_elem() ? 
                reinterpret_cast<const uint8_t*>(_free_storage) - b :
                reinterpret_cast<const uint8_t*>(p->first) - b;
        }

        void* next_storage_ptr(elem_ptr* p) noexcept
        {
            return p+1 == _free_elem ? _free_storage : (p + 1)->first;
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
