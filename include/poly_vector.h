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
#ifndef POLY_VECTOR_H_
#define POLY_VECTOR_H_

#include <memory>
#include <cstdint>
#include <utility>
#include <tuple>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <tuple>
#include <cmath>
#include "memory.h"

namespace estd
{
    namespace poly_vector_impl
    {
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

        template<typename... Ts>
        using or_type_t = typename OrType<Ts...>::type;
    }  //namespace poly_vector_impl

    template<class Allocator = std::allocator<uint8_t> >
    struct allocator_base : private Allocator
    {

        using allocator_traits = std::allocator_traits<Allocator>;
        using value_type = typename allocator_traits::value_type;
        using propagate_on_container_copy_assignment = typename allocator_traits::propagate_on_container_copy_assignment;
        using propagate_on_container_swap = typename allocator_traits::propagate_on_container_swap;
        using propagate_on_container_move_assignment = typename allocator_traits::propagate_on_container_move_assignment;
        using allocator_is_always_equal = ::estd::impl::allocator_is_always_equal_t<Allocator>;
        static_assert(std::is_same<value_type, uint8_t>::value, "requires a byte allocator");
        using allocator_type = Allocator;
        using void_ptr = void*;
        //////////////////////////////////////////////
        allocator_base(const Allocator& a = Allocator()) : Allocator(a), _storage{}, _end_storage{} {}
        allocator_base(size_t n, const Allocator& a = Allocator()) : Allocator(a), _storage{}, _end_storage{}
        {
            auto s = get_allocator_ref().allocate(n);
            _storage = s;
            _end_storage = s + n;
        }

        allocator_base(const allocator_base& a) :
            allocator_base(allocator_traits::select_on_container_copy_construction(a.get_allocator_ref()))
        {
            if (a.size())allocate(a.size());
        }
        allocator_base& operator=(const allocator_base& a)
        {
            return copy_assign_impl(a, propagate_on_container_copy_assignment{});
        }
        allocator_base(allocator_base&& a) noexcept :
        Allocator(std::move(a.get_allocator_ref())),
            _storage{ a._storage }, _end_storage{ a._end_storage }
        {
            a._storage = a._end_storage = nullptr;
        }
        allocator_base& operator=(allocator_base&& a) noexcept
        {
            return move_assign_impl(std::move(a), propagate_on_container_move_assignment{}, allocator_is_always_equal{});
        }

        allocator_base& move_assign_impl(allocator_base&& a, std::true_type, ...)
        {
            using std::swap;
            if (propagate_on_container_move_assignment::value) {
                swap(get_allocator_ref(), a.get_allocator_ref());
            }
            swap(a._storage, _storage);
            swap(a._end_storage, _end_storage);
            return *this;
        }
        allocator_base& move_assign_impl(allocator_base&& a, std::false_type, std::true_type)
        {
            return move_assign_impl(std::move(a), std::true_type{});
        }
        allocator_base& move_assign_impl(allocator_base&& a, std::false_type, std::false_type)
        {
            if (get_allocator_ref() != a.get_allocator_ref())
                return copy_assign_impl(a, std::false_type{});
            else
                return move_assign_impl(std::move(a), std::true_type{});
        }

        void swap(allocator_base& x) noexcept
        {
            using std::swap;
            if (propagate_on_container_swap::value) {
                swap(get_allocator_ref(), x.get_allocator_ref());
            }
            else if (!::estd::impl::allocator_is_always_equal_t<Allocator>::value &&
                get_allocator_ref() != x.get_allocator_ref()) {
                // Undefined behavior
                assert(0);
            }
            swap(_storage, x._storage);
            swap(_end_storage, x._end_storage);
        }
        allocator_base& copy_assign_impl(const allocator_base& a, std::true_type)
        {
            // Note: allocator copy assignment must not throw
            uint8_t* s = nullptr;
            if (a.size())
                s = a.get_allocator_ref().allocate(a.size());
            tidy();
            get_allocator_ref() = a.get_allocator_ref();
            _storage = s;
            _end_storage = s + a.size();
            return *this;
        }
        allocator_base& copy_assign_impl(const allocator_base& a, std::false_type)
        {
            uint8_t* s = nullptr;
            if (a.size())
                s = get_allocator_ref().allocate(a.size());
            tidy();
            _storage = s;
            _end_storage = s + a.size();
            return *this;
        }

        void allocate(size_t n)
        {
            auto s = get_allocator_ref().allocate(n);
            tidy();
            _storage = s;
            _end_storage = s + n;
        }

        allocator_type& get_allocator_ref()noexcept
        {
            return *this;
        }
        const allocator_type& get_allocator_ref()const noexcept
        {
            return *this;
        }

        void tidy() noexcept
        {
            get_allocator_ref().deallocate(reinterpret_cast<uint8_t*>(_storage), size());
            _storage = _end_storage = nullptr;
        }
        ~allocator_base()
        {
            tidy();
        }

        size_t size()const noexcept
        {
            return reinterpret_cast<uint8_t*>(_end_storage) -
                reinterpret_cast<uint8_t*>(_storage);
        }
        void* storage()noexcept
        {
            return _storage;
        }
        void* end_storage()noexcept
        {
            return _end_storage;
        }
        const void* storage()const noexcept
        {
            return _storage;
        }
        const void* end_storage()const noexcept
        {
            return _end_storage;
        }

        ////////////////////////////////
        void* _storage;
        void* _end_storage;
    };

    template<class IF, class CloningPolicy>
    struct poly_vector_elem_ptr : private CloningPolicy
    {
        using size_func_t = std::pair<size_t, size_t>();
        using policy_t = CloningPolicy;
        poly_vector_elem_ptr() : size{}, sf{} {}
        template<typename T, typename = std::enable_if_t< std::is_base_of< IF, std::decay_t<T> >::value > >
        explicit poly_vector_elem_ptr(T&& t, void* s = nullptr, IF* i = nullptr) noexcept :
            CloningPolicy(std::forward<T>(t)), ptr{ s, i }, sf{ &size_func<std::decay_t<T>> }
        {}
        poly_vector_elem_ptr(const poly_vector_elem_ptr& other) noexcept :
            CloningPolicy(other.policy()), ptr{ other.ptr }, sf{ other.sf }
        {}
        poly_vector_elem_ptr& operator=(const poly_vector_elem_ptr & rhs) noexcept
        {
            policy() = rhs.policy();
            ptr = rhs.ptr;
            sf = rhs.sf;
        }
        policy_t& policy()noexcept
        {
            return *this;
        }
        const policy_t& policy()const noexcept
        {
            return *this;
        }
        size_t size()const noexcept
        {
            return sf().first;
        }
        size_t align() const noexcept
        {
            return sf().second;
        }
        template<typename T>
        static std::pair<size_t, size_t> size_func()
        {
            return std::make_pair(sizeof(T), alignof(T));
        }
        ~poly_vector_elem_ptr()
        {
            ptr.first = ptr.second = nullptr;
            sf = nullptr;
        }

        std::pair<void*, IF*> ptr;
    private:
        size_func_t* sf;
    };

    template<class IF>
    struct virtual_cloning_policy
    {
        static constexpr bool nem = noexcept(std::declval<IF*>()->move(std::declval<void*>()));
        using noexcept_moveable = std::integral_constant<bool, nem>;
        virtual_cloning_policy() = default;
        template<typename T>
        virtual_cloning_policy(T&& t) {};
        IF* clone(IF* obj, void* dest) const
        {
            return obj->clone(dest);
        }
        IF* move(IF* obj, void* dest) const noexcept(nem)
        {
            return obj->move(dest);
        }
    };

    template<class IF>
    struct no_cloning_policy
    {
        struct exception : public std::exception
        {
            const char* const what() const noexcept override
            {
                return "cloning attempt with no_cloning_policy";
            }
        };
        using  noexcept_moveable = std::false_type;
        no_cloning_policy() = default;
        template<typename T>
        no_cloning_policy(T&& t) {};
        IF* clone(IF* obj, void* dest) const
        {
            throw exception{};
        }
        IF* move(IF* obj, void* dest) const
        {
            throw exception{};
        }
    };

    template<class Interface, typename NoExceptMoveAble = std::true_type>
    struct delegate_cloning_policy
    {
        enum Operation {
            Clone,
            Move
        };

        typedef Interface* (*clone_func_t)(Interface* obj, void* dest, Operation);
        using  noexcept_moveable = NoExceptMoveAble;
        delegate_cloning_policy() :cf{} {}

        template <
            typename T,
            typename = std::enable_if_t<!std::is_same<delegate_cloning_policy, std::decay_t<T>>::value >
        >
            delegate_cloning_policy(T&& t) noexcept :
        cf(&delegate_cloning_policy::clone_func< std::decay_t<T> >)
        {
            // this ensures, that if policy requires noexcept move construction that a given T type also has it
            static_assert(!noexcept_moveable::value || std::is_nothrow_move_constructible<std::decay_t<T>>::value,
                "delegate cloning policy requires noexcept move constructor");
        };

        delegate_cloning_policy(const delegate_cloning_policy & other) noexcept : cf{ other.cf } {}

        template<class T>
        static Interface* clone_func(Interface* obj, void* dest, Operation op)
        {
            if (op == Clone)
                return new(dest) T(*static_cast<const T*>(obj));
            else
                return new(dest) T(std::move(*static_cast<const T*>(obj)));
        }

        Interface* clone(Interface* obj, void* dest) const
        {
            return cf(obj, dest, Clone);
        }

        Interface* move(Interface* obj, void* dest) const noexcept(noexcept_moveable::value)
        {
            return cf(obj, dest, Move);
        }
        /////////////////////////
    private:
        clone_func_t cf;
    };


    template<class IF, class CP>
    class poly_vector_iterator
    {
    public:
        using void_ptr = void*;
        using interface_type = IF;
        using interface_ptr = interface_type*;
        using elem_ptr = poly_vector_elem_ptr<IF, CP>;

        poly_vector_iterator() : curr{} {}
        explicit poly_vector_iterator(elem_ptr* p) : curr{ p } {}
        poly_vector_iterator(const poly_vector_iterator&) = default;
        poly_vector_iterator& operator=(const poly_vector_iterator&) = default;
        ~poly_vector_iterator() = default;

        IF* operator->() noexcept
        {
            return curr->ptr.second;
        }
        IF& operator*()noexcept
        {
            return *curr->ptr.second;
        }
        const IF* operator->() const noexcept
        {
            return curr->ptr.second;
        }
        const IF& operator*()const noexcept
        {
            return *curr->ptr.second;
        }

        poly_vector_iterator& operator++()noexcept
        {
            ++curr;
            return *this;
        }
        poly_vector_iterator operator++(int)noexcept
        {
            poly_vector_iterator i(*this);
            ++curr;
            return i;
        }
        poly_vector_iterator& operator--()noexcept
        {
            --curr;
            return *this;
        }
        poly_vector_iterator operator--(int)noexcept
        {
            poly_vector_iterator i(*this);
            --curr;
            return i;
        }
        bool operator==(const poly_vector_iterator& rhs)const noexcept
        {
            return curr == rhs.curr;
        }
        bool operator!=(const poly_vector_iterator& rhs)const noexcept
        {
            return curr != rhs.curr;
        }

    private:
        elem_ptr* curr;
    };

    template<
        class IF,
        class Allocator = std::allocator<uint8_t>,
        class CloningPolicy = virtual_cloning_policy<IF>
    >
        class poly_vector :
        private allocator_base<
        typename std::allocator_traits<Allocator>::template rebind_alloc<uint8_t>
        >
    {
    public:
        ///////////////////////////////////////////////
        // Member types
        ///////////////////////////////////////////////
        using interface_type = std::decay_t<IF>;
        using interface_ptr = interface_type*;
        using const_interface_ptr = const interface_type*;
        using interface_reference = interface_type&;
        using const_interface_reference = const interface_type&;
        using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<uint8_t>;
        using allocator_traits = typename allocator_base::allocator_traits;
        using void_ptr = void*;
        using size_type = std::size_t;
        using iterator = poly_vector_iterator<interface_type, CloningPolicy>;
        using const_iterator = poly_vector_iterator<const interface_type, const CloningPolicy>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using move_is_noexcept_t = poly_vector_impl::or_type_t<
            typename allocator_traits::propagate_on_container_move_assignment,
            estd::impl::allocator_is_always_equal_t<allocator_type>
        >;
        static constexpr auto default_avg_size = 4 * sizeof(void*);
        static_assert(std::is_polymorphic<interface_type>::value, "interface_type is not polymorphic");
        ///////////////////////////////////////////////
        // Ctors,Dtors & assignment
        ///////////////////////////////////////////////
        poly_vector() : _free_elem{}, _begin_storage{}, _free_storage{} {}
        explicit poly_vector(const allocator_type& alloc) : allocator_base<allocator_type>(alloc),
            _free_elem{}, _begin_storage{}, _free_storage{} {};
        poly_vector(const poly_vector& other) : allocator_base<allocator_type>(other.base()),
            _free_elem{ begin_elem() }, _begin_storage{ begin_elem() + other.size() },
            _free_storage{ _begin_storage }
        {
            set_ptrs(other.poly_uninitialized_copy(begin_elem(), other.size()));
        }
        poly_vector(poly_vector&& other) : allocator_base<allocator_type>(std::move(other.base())),
            _free_elem{ other._free_elem }, _begin_storage{ other._begin_storage },
            _free_storage{ other._free_storage }
        {
            other._begin_storage = other._free_storage = other._free_elem = nullptr;
        }
        ~poly_vector()
        {
            tidy();
        }
        poly_vector& operator=(const poly_vector& rhs)
        {
            if (this != &rhs) {
                return copy_assign_impl(rhs);
            }
            return *this;
        }
        poly_vector& operator=(poly_vector&& rhs) noexcept(move_is_noexcept_t::value)
        {
            if (this != &rhs) {
                return move_assign_impl(std::move(rhs), move_is_noexcept_t{});
            }
            return *this;
        }
        ///////////////////////////////////////////////
        // Modifiers
        ///////////////////////////////////////////////
        template<typename T>
        std::enable_if_t<std::is_base_of<interface_type, std::decay_t<T>>::value>
            push_back(T&& obj)
        {
            using TT = std::decay_t<T>;
            constexpr auto s = sizeof(TT);
            constexpr auto a = alignof(TT);
            if (end_elem() == _begin_storage || !can_construct_new_elem(s, a)) {
                push_back_new_elem_w_storage_increase(std::forward<T>(obj));
            }
            else {
                push_back_new_elem(std::forward<T>(obj));
            }
        }
        void pop_back()noexcept
        {
            back().~IF();
            _free_storage = size() > 1 ? reinterpret_cast<uint8_t*>(begin_elem()[size() - 2].ptr.first) +
                begin_elem()[size() - 2].size() : _begin_storage;
            begin_elem()[size() - 1].~poly_vector_elem_ptr();
            _free_elem = &begin_elem()[size() - 1];
        }
        void clear() noexcept
        {
            for (auto src = begin_elem(); src != _free_elem; ++src) {
                src->ptr.second->~IF();
            }
            _free_elem = begin_elem();
            _free_storage = _begin_storage;
        }
        void swap(poly_vector& x) noexcept
        {
            using std::swap;
            base().swap(x.base());
            swap(_free_elem, x._free_elem);
            swap(_begin_storage, x._begin_storage);
            swap(_free_storage, x._free_storage);
        }
        // TODO: insert, erase,emplace, emplace_back, assign?
        ///////////////////////////////////////////////
        // Iterators
        ///////////////////////////////////////////////
        iterator begin()noexcept
        {
            return iterator(begin_elem());
        }
        iterator end()noexcept
        {
            return iterator(end_elem());
        }
        const_iterator begin()const noexcept
        {
            return iterator(begin_elem());
        }
        const_iterator end()const noexcept
        {
            return iterator(end_elem());
        }
        reverse_iterator rbegin()noexcept
        {
            return reverse_iterator(end() - 1);
        }
        reverse_iterator rend()noexcept
        {
            return reverse_iterator(begin() - 1);
        }
        const_iterator rbegin()const noexcept
        {
            return iterator(begin_elem());
        }
        const_iterator rend()const noexcept
        {
            return iterator(end_elem());
        }
        ///////////////////////////////////////////////
        // Capacity
        ///////////////////////////////////////////////
        size_t size()const noexcept
        {
            return _free_elem - begin_elem();
        }
        std::pair<size_t, size_t> sizes() const noexcept
        {
            return std::make_pair(size(), avg_obj_size());
        }
        size_type capacity() const noexcept
        {
            return storage_size(begin_elem(), _begin_storage) / sizeof(elem_ptr);
        }
        std::pair<size_t, size_t> capacities() const noexcept
        {
            return std::make_pair(capacity(), storage_size(_begin_storage, _end_storage));
        }
        bool empty() const noexcept
        {
            return begin_elem() == _free_elem;
        }
        size_type max_size() const noexcept
        {
            auto avg = avg_obj_size() ? avg_obj_size() : 4 * sizeof(void_ptr);
            return allocator_traits::max_size(get_allocator_ref()) /
                (sizeof(elem_ptr) + avg);
        }
        void reserve(size_type n, size_type avg_size = avg_obj_size())
        {
            constexpr size_t default_avg_size = 4 * sizeof(void*);
            if (n < capacity()) return;
            if (n > max_size())throw std::length_error("poly_vector reserve size too big");
            auto avg_s = avg_size ? avg_size : default_avg_size;
            increase_storage(*this, *this, n, avg_s, alignof(std::max_align_t), select_copy_method());
        }
        void reserve(std::pair<size_t, size_t> s)
        {
            reserve(s.first, s.second);
        }
        // TODO: void shrink_to_fit();
        ///////////////////////////////////////////////
        // Element access
        ///////////////////////////////////////////////
        interface_reference operator[](size_t n) noexcept
        {
            return *begin_elem()[n].ptr.second;
        }
        const_interface_reference operator[](size_t n)const noexcept
        {
            return *begin_elem()[n].ptr.second;
        }
        interface_reference at(size_t n)
        {
            if (n >= size()) throw std::out_of_range{ "poly_vector out of range access" };
            return (*this)[n];
        }
        const_interface_reference at(size_t n)const
        {
            if (n >= size()) throw std::out_of_range{ "poly_vector out of range access" };
            return (*this)[n];
        }
        interface_reference front() noexcept
        {
            return (*this)[0];
        }
        const_interface_reference front()const noexcept
        {
            return (*this)[0];
        }
        interface_reference back() noexcept
        {
            return (*this)[size() - 1];
        }
        const_interface_reference back()const noexcept
        {
            return (*this)[size() - 1];
        }
        ////////////////////////////
        // Misc.
        ////////////////////////////
        allocator_type get_allocator() const noexcept
        {
            return my_base::get_allocator_ref();
        }
    private:
        using my_base = allocator_base<allocator_type>;
        using elem_ptr = poly_vector_elem_ptr<interface_type, CloningPolicy>;
        using alloc_descr_t = std::pair<bool, std::pair<size_t, size_t>>;
        using poly_copy_descr = std::tuple<elem_ptr*, elem_ptr*, void*>;
        typedef poly_copy_descr(poly_vector::*copy_mem_fun)(elem_ptr*, size_t) const;

        static alloc_descr_t make_alloc_descr(bool b, size_t size, size_t align)noexcept
        {
            return std::make_pair(b, std::make_pair(size, align));
        }

        static void_ptr next_aligned_storage(void*p, size_t align) noexcept
        {
            auto fs = reinterpret_cast<uintptr_t>(p);
            return reinterpret_cast<void_ptr>(((fs + align - 1) / align)*align);
        }

        static void* next_storage_for_object(void* s, size_t size, size_t align) noexcept
        {
            s = next_aligned_storage(s, align);
            s = reinterpret_cast<uint8_t*>(s) + size;
            return s;
        }

        static size_t estimate_excess(void* s, void* e, alloc_descr_t n, size_t avg) noexcept
        {
            auto es = storage_size(reinterpret_cast<uint8_t*>(s) + n.second.first*avg, e);
            auto en = (es + (sizeof(elem_ptr) + avg) - 1) / (sizeof(elem_ptr) + avg);
            return std::max(size_t(1), en);
        }

        static size_t storage_size(const void* b, const void* e) noexcept
        {
            return reinterpret_cast<const uint8_t*>(e) -
                reinterpret_cast<const uint8_t*>(b);
        }

        static void increase_storage(const poly_vector& src, poly_vector& dst,
            size_t desired_size, size_t curr_elem_size = 0, size_t align = 1,
            copy_mem_fun func = &poly_vector::poly_uninitialized_copy)
        {
            alloc_descr_t n{ false,std::make_pair(desired_size,size_type(0)) };
            my_base s{};
            while (!n.first) {
                n.second = src.calc_increased_storage_size(n.second.first, curr_elem_size, align);
                my_base a(n.second.second, dst.get_allocator_ref());
                s.swap(a);
                n = src.validate_layout(s.storage(), s.end_storage(), n, curr_elem_size, align);
                if (!n.first) n.second.first *= 2;
            }
            dst.obtain_storage(src, dst, std::move(s), n.second.first, func);
        }

        static void obtain_storage(const poly_vector& src, poly_vector& dst, my_base&& a, size_t n,
            copy_mem_fun func)
        {
            auto ret = (src.*func)(reinterpret_cast<elem_ptr*>(a.storage()), n);
            dst.tidy();
            dst.base().swap(a);
            dst.set_ptrs(ret);
        }

        static copy_mem_fun select_copy_method() {
            if (typename CloningPolicy::noexcept_moveable::value) {
                return &poly_vector::poly_uninitialized_move;
            }
            else {
                return &poly_vector::poly_uninitialized_copy;
            }
        }

        my_base& base()noexcept
        {
            return *this;
        }
        const my_base& base()const noexcept
        {
            return *this;
        }

        std::pair<size_type, size_type> calc_increased_storage_size(
            size_t desired_size, size_t curr_elem_size, size_t align) const noexcept
        {
            const auto s = new_avg_obj_size(std::max(curr_elem_size, align), align);
            auto begin = reinterpret_cast<uint8_t*>(nullptr);
            const auto first_align = !empty() ? begin_elem()->align() : size_t(1);
            auto end = reinterpret_cast<uint8_t*>(
                next_aligned_storage(begin + desired_size * sizeof(elem_ptr), first_align));
            end += storage_size(_begin_storage, end_storage());
            end = reinterpret_cast<uint8_t*>(next_aligned_storage(end, align));
            end += (desired_size - size())*s;
            return std::make_pair(desired_size, storage_size(begin, end));
        }

        alloc_descr_t validate_layout(void* const start, void* const end, const alloc_descr_t n,
            const size_t size, const size_t align) const noexcept
        {
            uint8_t* const new_begin_storage =
                reinterpret_cast<uint8_t*>(start) + n.second.first * sizeof(elem_ptr);
            void* new_end_storage = new_begin_storage;
            for (auto p = begin_elem(); p != end_elem() && new_end_storage <= end; ++p) {
                new_end_storage = next_storage_for_object(new_end_storage, p->size(), p->align());
            }
            // at least one new object should be constructed
            new_end_storage = next_storage_for_object(new_end_storage, size, align);
            //return with previous alloc descriptor
            if (new_end_storage > end)
                return n;
            //do not re-pivot begin_storage if there is not too much headroom or enough
            if (storage_size(new_begin_storage, end) / new_avg_obj_size(size, align) <= n.second.first ||
                storage_size(new_end_storage, end) < new_avg_obj_size(size, align)) {
                return std::make_pair(true, std::make_pair(n.second.first, n.second.second));
            }
            // estimate excess elem no
            auto new_size = n.second.first +
                estimate_excess(new_begin_storage, end, n, new_avg_obj_size(size, align));
            // try to re-pivot storage
            return validate_layout(start, end, std::make_pair(true,
                std::make_pair(new_size, n.second.second)), size, align);
        }

        poly_copy_descr poly_uninitialized_copy(elem_ptr* dst, size_t n) const
        {
            return poly_uninitialized(dst, n,
                [](const elem_ptr* e, elem_ptr* d)->Interface* {
                return e->policy().clone(e->ptr.second, d->ptr.first);
            });
        }

        poly_copy_descr poly_uninitialized_move(elem_ptr* dst, size_t n) const
            noexcept(typename elem_ptr::policy_t::noexcept_moveable::value)
        {
            return poly_uninitialized(dst, n,
                [](const elem_ptr* e, elem_ptr* d)->Interface* {
                return e->policy().move(e->ptr.second, d->ptr.first);
            });
        }

        template<typename F>
        poly_copy_descr poly_uninitialized(elem_ptr* dst, size_t n, F copy_func) const
        {
            auto dst_begin = dst;
            const auto storage_begin = dst + n;
            void_ptr dst_storage = storage_begin;
            try {
                for (auto elem = begin_elem(); elem != _free_elem; ++elem, ++dst) {
                    dst = new (dst) elem_ptr(*elem);
                    dst->ptr.first = next_aligned_storage(dst_storage, elem->align());
                    dst->ptr.second = copy_func(elem, dst);
                    dst_storage = reinterpret_cast<uint8_t*>(dst->ptr.first) + dst->size();
                }
                return std::make_tuple(dst, storage_begin, dst_storage);
            }
            catch (...) {
                for (; dst != dst_begin; --dst) {
                    dst->ptr.second->~IF();
                }
                throw;
            }
        }

        poly_vector& copy_assign_impl(const poly_vector& rhs)
        {
            return copy_move_helper(rhs, &poly_vector::poly_uninitialized_copy);
        }

        poly_vector& move_assign_impl(poly_vector&& rhs, std::true_type) noexcept
        {
            using std::swap;
            base() = std::move(rhs.base());
            swap_ptrs(std::move(rhs));
            return *this;
        }

        poly_vector& move_assign_impl(poly_vector&& rhs, std::false_type)
        {
            if (base().get_allocator_ref() == rhs.base().get_allocator_ref()) {
                return move_assign_impl(std::move(rhs), std::true_type{});
            }
            else
                return copy_move_helper(std::move(rhs), &poly_vector::poly_uninitialized_move);
        }


        template<typename T>
        poly_vector& copy_move_helper(T&& rhs, copy_mem_fun func)
        {
            tidy();
            if (std::is_reference<T>::value) {
                base().get_allocator_ref() = rhs.base().get_allocator_ref();
            }
            else {
                base().get_allocator_ref() = std::move(rhs.base().get_allocator_ref());
            }
            increase_storage(rhs, *this, rhs.size(), 0, 1, func);
            return *this;
        }

        void tidy() noexcept
        {
            for (auto src = begin_elem(); src != _free_elem; ++src) {
                src->ptr.second->~IF();
            }
            _begin_storage = _free_storage = _free_elem = nullptr;
            my_base::tidy();
        }

        void set_ptrs(poly_copy_descr p)
        {
            _free_elem = std::get<0>(p);
            _begin_storage = std::get<1>(p);
            _free_storage = std::get<2>(p);
        }

        void swap_ptrs(poly_vector&& rhs)
        {
            using std::swap;
            swap(_free_elem, rhs._free_elem);
            swap(_begin_storage, rhs._begin_storage);
            swap(_free_storage, rhs._free_storage);
        }

        template<class T>
        void push_back_new_elem(T&& obj)
        {
            using TT = std::decay_t<T>;
            constexpr auto s = sizeof(TT);
            constexpr auto a = alignof(TT);
            auto nas = next_aligned_storage(a);

            assert(can_construct_new_elem(s, a));

            _free_elem = new (_free_elem) elem_ptr(std::forward<T>(obj),
                nas, new (nas) TT(std::forward<T>(obj)));
            _free_storage = reinterpret_cast<uint8_t*>(_free_elem->ptr.first) + s;
            ++_free_elem;
        }

        template<class T>
        void push_back_new_elem_w_storage_increase(T&& obj)
        {
            using TT = std::decay_t<T>;
            constexpr auto s = sizeof(TT);
            constexpr auto a = alignof(TT);
            using select = std::conditional_t<std::is_reference<T>::value,
                std::false_type,
                typename CloningPolicy::noexcept_moveable
            >;
            //////////////////////////////////////////
            poly_vector v(base().get_allocator_ref());
            auto new_size = std::max(size() * 2, size_t(1));
            alloc_descr_t n{ false,std::make_pair(new_size,size_type(0)) };
            while (!n.first) {
                v.reserve(n.second.first, new_avg_obj_size(s, a));
                n = validate_layout(v.storage(), v.end_storage(), n, s, a);
                if (!n.first) n.second.first *= 2;
            }
            // move if rvalue ref and cloning policy move is noexcept
            push_back_new_elem_w_storage_increase_copy(v, select{});
            v.push_back_new_elem(std::forward<T>(obj));
            this->swap(v);
        }

        void push_back_new_elem_w_storage_increase_copy(poly_vector& v, std::true_type) {
            v.set_ptrs(poly_uninitialized_move(v.begin_elem(), v.capacity()));
        }
        void push_back_new_elem_w_storage_increase_copy(poly_vector& v, std::false_type) {
            v.set_ptrs(poly_uninitialized_copy(v.begin_elem(), v.capacity()));
        }


        bool can_construct_new_elem(size_t s, size_t align) noexcept
        {
            auto free = reinterpret_cast<uint8_t*>(next_aligned_storage(_free_storage, align));
            return free + s <= end_storage();
        }
        void_ptr next_aligned_storage(size_t align) const noexcept
        {
            return next_aligned_storage(_free_storage, align);
        }
        size_t new_avg_obj_size(size_t new_object_size, size_t align)const noexcept
        {
            return (storage_size(_begin_storage, reinterpret_cast<uint8_t*>(next_aligned_storage(align)) + new_object_size)
                + this->size()) / (this->size() + 1);
        }
        size_t avg_obj_size(size_t align = 1)const noexcept
        {
            return storage_size() ? (storage_size(_begin_storage, next_aligned_storage(align)) + size() - 1) / size() : 0;
        }
        size_t storage_size()const noexcept
        {
            return storage_size(_begin_storage, _free_storage);
        }
        elem_ptr* begin_elem()noexcept
        {
            return reinterpret_cast<elem_ptr*>(storage());
        }
        elem_ptr* end_elem()noexcept
        {
            return reinterpret_cast<elem_ptr*>(_free_elem);
        }
        const elem_ptr* begin_elem()const noexcept
        {
            return reinterpret_cast<const elem_ptr*>(storage());
        }
        const elem_ptr* end_elem()const noexcept
        {
            return reinterpret_cast<const elem_ptr*>(_free_elem);
        }
        ////////////////////////////
        // Members
        ////////////////////////////
        elem_ptr* _free_elem;
        void* _begin_storage;
        void* _free_storage;
    };

    template<
        class IF,
        class Allocator,
        class CloningPolicy
    >
        void swap(poly_vector<IF, Allocator, CloningPolicy>& lhs, poly_vector<IF, Allocator, CloningPolicy>& rhs) noexcept
    {
        lhs.swap(rhs);
    }
    // TODO: relational operators
}  // namespace estd

#endif  //POLY_VECTOR_H_
