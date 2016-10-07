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
#include <type_traits>
#include <iterator>
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

        template<typename T>
        struct TD;


        template<class TT>
        struct has_noexcept_movable{
            template<class U>
            static std::true_type test(U&&,typename std::decay_t<U>::noexcept_movable *);
            static std::false_type test(...);
            static constexpr bool value =
                    std::is_same<std::true_type,decltype(test(std::declval<TT>(),nullptr))>::value;
        };

        template<class TT,bool HasType>
        struct is_noexcept_movable{
            using type = typename TT::noexcept_movable;
        };
        template<class TT>
        struct is_noexcept_movable<TT,false>{
            using type = std::false_type;
        };

        template<class TT>
        using is_noexcept_movable_t = typename is_noexcept_movable<TT,has_noexcept_movable<TT>::value>::type;

        template<class T,class IF,class A>
        struct is_cloning_policy_impl{
            using void_ptr = typename std::allocator_traits<A>::void_pointer;
            using pointer = typename std::allocator_traits<A>::pointer;

            template<class TT>
            struct has_clone_impl{
                template<class U>
                static decltype(std::declval<U>()
                        .clone(std::declval<A>(),std::declval<pointer>(),std::declval<void_ptr>()))* test(U&&);

                static std::false_type test(...);

                static constexpr bool value = std::is_same<decltype(test(std::declval<TT>())),pointer*>::value;
            };
            template<class TT>
            struct has_move_impl{
                template<class U>
                static decltype(std::declval<U>()
                        .move(std::declval<A>(),std::declval<pointer>(),std::declval<void_ptr>()))* test(U&&);
                static std::false_type test(...);
                static constexpr bool value = std::is_same<pointer*,decltype(test(std::declval<TT>()))>::value;
            };

            using has_move_t = std::integral_constant<bool,has_move_impl<T>::value>;
            using has_clone_t = std::integral_constant<bool,has_clone_impl<T>::value>;
            // TODO(fecjanky): provide traits defaults for Allocator type, pointer and void_pointer types
            static constexpr bool value =
                    std::is_same<IF,typename std::pointer_traits<pointer>::element_type>::value &&
                    std::is_nothrow_constructible<T>::value &&
                    std::is_nothrow_copy_constructible<T>::value &&
                    std::is_nothrow_copy_assignable<T>::value &&
                    (has_clone_t::value || (has_move_t::value && is_noexcept_movable_t<T>::value));
        };
    }  //namespace poly_vector_impl

    template<class Policy,class Interface,class Allocator>
    using is_cloning_policy = std::integral_constant<bool,poly_vector_impl::is_cloning_policy_impl<Policy,Interface,Allocator>::value>;

    template<class Policy,class Interface,class Allocator>
    struct cloning_policy_traits{
        static_assert(is_cloning_policy<Policy,Interface,Allocator>::value,"Policy is not a cloning policy");
        using noexcept_movable = poly_vector_impl::is_noexcept_movable_t<Policy>;
        using pointer = typename Policy::pointer;
        using void_pointer = typename Policy::void_pointer;
        using allocator_type = typename Policy::allocator_type;
        static pointer move(const Policy& p, const allocator_type& a, pointer obj, void_pointer dest) noexcept(noexcept_movable::value)
        {
            using policy_impl = poly_vector_impl::is_cloning_policy_impl<Policy,Interface,Allocator>;
            return move_impl(p,a,obj,dest,typename policy_impl::has_move_t{});
        }
    private:
        static pointer move_impl(const Policy& p, const allocator_type& a, pointer obj, void_pointer dest, std::true_type) noexcept(noexcept_movable::value)
        {
            return p.move(a,obj,dest);
        }
        static pointer move_impl(const Policy& p, const allocator_type& a, pointer obj, void_pointer dest,std::false_type)
        {
            return p.clone(a,obj,dest);
        }

    };


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
        using void_pointer = typename allocator_traits::void_pointer;
        using const_void_pointer = typename allocator_traits::const_void_pointer;
        using difference_type = typename  allocator_traits::difference_type;
        using pointer = typename allocator_traits::pointer ;
        using const_pointer = typename allocator_traits::const_pointer;
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

        allocator_base(allocator_base&& a) noexcept :
                Allocator(std::move(a.get_allocator_ref())),
                _storage{ a._storage }, _end_storage{ a._end_storage }
        {
            a._storage = a._end_storage = nullptr;
        }

        allocator_base& operator=(const allocator_base& a)
        {
            return copy_assign_impl(a, propagate_on_container_copy_assignment{});
        }

        allocator_base& operator=(allocator_base&& a)
            noexcept(propagate_on_container_move_assignment::value || allocator_is_always_equal::value)
        {
            return move_assign_impl(std::move(a), propagate_on_container_move_assignment{}, allocator_is_always_equal{});
        }

        allocator_base& move_assign_impl(allocator_base&& a, std::true_type, ...) noexcept
        {
            using std::swap;
            tidy();
            if (propagate_on_container_move_assignment::value) {
                // must not throw
                get_allocator_ref() = std::move(a.get_allocator_ref());
            }
            swap(a._storage, _storage);
            swap(a._end_storage, _end_storage);
            return *this;
        }

        allocator_base& move_assign_impl(allocator_base&& a, std::false_type, std::true_type) noexcept
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

        allocator_base& copy_assign_impl(const allocator_base& a, std::true_type)
        {
            // Note: allocator copy assignment must not throw
            pointer s = nullptr;
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
            pointer s = nullptr;
            if (a.size())
                s = get_allocator_ref().allocate(a.size());
            tidy();
            _storage = s;
            _end_storage = s + a.size();
            return *this;
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
            get_allocator_ref().deallocate(static_cast<pointer>(_storage), size());
            _storage = _end_storage = nullptr;
        }

        ~allocator_base()
        {
            tidy();
        }

        difference_type size()const noexcept
        {
            return static_cast<pointer>(_end_storage) -
                static_cast<pointer>(_storage);
        }

        void_pointer storage()noexcept
        {
            return _storage;
        }
        void_pointer end_storage()noexcept
        {
            return _end_storage;
        }
        const_void_pointer storage()const noexcept
        {
            return _storage;
        }
        const_void_pointer end_storage()const noexcept
        {
            return _end_storage;
        }
        template<typename PointerType>
        void destroy(PointerType obj) const{
            using T = std::decay_t<decltype(*obj)>;
            using traits = typename allocator_traits::template rebind_traits <T>;
            static_assert(std::is_same<PointerType, typename traits::pointer>::value,"Invalid pointer type");
            typename traits::allocator_type a(get_allocator_ref());
            traits::destroy(a,obj);
        }
        template<typename PointerType,typename... Args>
        PointerType construct(PointerType storage,Args&&... args) const{
            using T = std::decay_t<decltype(*storage)>;
            using traits = typename allocator_traits::template rebind_traits <T>;
            static_assert(std::is_same<PointerType, typename traits::pointer>::value,"Invalid pointer type");
            typename traits::allocator_type a(get_allocator_ref());
            traits::construct(a,storage,std::forward<Args>(args)...);
            return storage;
        }
        ////////////////////////////////
        void_pointer _storage;
        void_pointer _end_storage;
    };

    template<class Allocator,class CloningPolicy>
    struct poly_vector_elem_ptr : private CloningPolicy
    {
        using IF = typename  std::allocator_traits<Allocator>::value_type;
        using void_pointer = typename std::allocator_traits<Allocator>::void_pointer;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using size_func_t = std::pair<size_t, size_t>();
        using policy_t = CloningPolicy;
        poly_vector_elem_ptr() : ptr{}, sf{} {}

        template<typename T, typename = std::enable_if_t< std::is_base_of< IF, std::decay_t<T> >::value > >
        explicit poly_vector_elem_ptr(T&& t, void_pointer s = nullptr, pointer i = nullptr) noexcept :
            CloningPolicy(std::forward<T>(t)), ptr{ s, i }, sf{ &poly_vector_elem_ptr::size_func<std::decay_t<T>>}
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

        std::pair<void_pointer, pointer> ptr;
    private:
        size_func_t* sf;
    };

    template<class IF,class Allocator = std::allocator<IF> >
    struct virtual_cloning_policy
    {
        static constexpr bool nem = noexcept(std::declval<IF*>()->move(std::declval<Allocator>(),std::declval<void*>()));
        using noexcept_movable = std::integral_constant<bool, nem>;
        using void_pointer = typename std::allocator_traits<Allocator>::void_pointer;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using allocator_type = Allocator;

        virtual_cloning_policy() = default;
        template<typename T>
        virtual_cloning_policy(T&& t) {};
        pointer clone(const Allocator& a,pointer obj, void_pointer dest) const
        {
            return obj->clone(a,dest);
        }
        pointer move(const Allocator& a,pointer obj, void_pointer dest) const noexcept(nem)
        {
            return obj->move(a,dest);
        }
    };

    template<class IF,class Allocator = std::allocator<IF> >
    struct no_cloning_policy
    {
        struct exception : public std::exception
        {
            const char* what() const noexcept override
            {
                return "cloning attempt with no_cloning_policy";
            }
        };
        using noexcept_movable = std::false_type;
        using void_pointer = typename std::allocator_traits<Allocator>::void_pointer;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using allocator_type = Allocator;
        no_cloning_policy() = default;
        template<typename T>
        no_cloning_policy(T&& t) {};
        pointer clone(const Allocator& a,pointer obj, void_pointer dest) const
        {
            throw exception{};
        }
    };

    template<class Interface,class Allocator = std::allocator<Interface>, typename NoExceptmovable = std::true_type>
    struct delegate_cloning_policy
    {
        enum Operation {
            Clone,
            Move
        };

        typedef Interface* (*clone_func_t)(const Allocator& a,Interface* obj, void* dest, Operation);
        using noexcept_movable = NoExceptmovable;
        using void_pointer = typename std::allocator_traits<Allocator>::void_pointer;
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
        using allocator_type = Allocator;

        delegate_cloning_policy() noexcept :cf{} {}

        template <
            typename T,
            typename = std::enable_if_t<!std::is_same<delegate_cloning_policy, std::decay_t<T>>::value >
        >
            explicit delegate_cloning_policy(T&& t) noexcept :
        cf(&delegate_cloning_policy::clone_func< std::decay_t<T> >)
        {
            // this ensures, that if policy requires noexcept move construction that a given T type also has it
            static_assert(!noexcept_movable::value || std::is_nothrow_move_constructible<std::decay_t<T>>::value,
                "delegate cloning policy requires noexcept move constructor");
        };

        delegate_cloning_policy(const delegate_cloning_policy & other) noexcept : cf{ other.cf } {}

        template<class T>
        static pointer clone_func(const Allocator& a,pointer obj, void_pointer dest, Operation op)
        {
            using traits = typename std::allocator_traits<Allocator>::template rebind_traits <T>;
            using obj_pointer = typename traits::pointer;
            using obj_const_pointer = typename traits::const_pointer;

            typename traits::allocator_type alloc(a);
            if (op == Clone) {
                traits::construct(alloc, static_cast<obj_pointer>(dest), *static_cast<obj_const_pointer>(obj));
            } else {
                traits::construct(alloc, static_cast<obj_pointer>(dest), std::move(*static_cast<obj_pointer>(obj)));
            }
            return static_cast<obj_pointer>(dest);
        }

        pointer clone(const Allocator& a,pointer obj, void_pointer dest) const
        {
            return cf(a,obj, dest, Clone);
        }

        pointer move(const Allocator& a,pointer obj, void_pointer dest) const noexcept(noexcept_movable::value)
        {
            return cf(a,obj, dest, Move);
        }
        /////////////////////////
    private:
        clone_func_t cf;
    };


    template<class IF,class Allocator,class CP>
    class poly_vector_iterator
    {
    public:
        using elem = poly_vector_elem_ptr<Allocator, CP>;
        using void_ptr = typename std::allocator_traits<Allocator>::void_pointer;
        using pointer = typename std::allocator_traits<Allocator>::pointer ;
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
        using reference = decltype(*std::declval<pointer>());
        using const_reference = decltype(*std::declval<const_pointer>());
        using elem_ptr = typename std::pointer_traits<pointer>::template rebind <elem>;

        poly_vector_iterator() : curr{} {}
        explicit poly_vector_iterator(elem_ptr p) : curr{ p } {}
        poly_vector_iterator(const poly_vector_iterator&) = default;
        poly_vector_iterator& operator=(const poly_vector_iterator&) = default;
        ~poly_vector_iterator() = default;

        pointer operator->() noexcept
        {
            return curr->ptr.second;
        }
        reference operator*()noexcept
        {
            return *curr->ptr.second;
        }
        const_pointer operator->() const noexcept
        {
            return curr->ptr.second;
        }
        const_reference operator*()const noexcept
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
        elem_ptr curr;
    };

    template<
        class IF,
        class Allocator = std::allocator<IF>,
        /// implicit noexcept_movability when using defaults of delegate cloning policy
        class CloningPolicy = delegate_cloning_policy<IF,Allocator>
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
        using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<uint8_t>;
        using interface_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<IF>;
        using my_base = allocator_base<allocator_type>;
        using interface_type = std::decay_t<IF>;
        using interface_pointer = typename interface_allocator_type ::pointer;
        using const_interface_pointer = typename interface_allocator_type ::const_pointer ;
        using interface_reference = typename interface_allocator_type::reference ;
        using const_interface_reference = typename interface_allocator_type::const_reference;
        using allocator_traits = std::allocator_traits<allocator_type>;
        using pointer = typename my_base::pointer;
        using const_pointer = typename my_base::const_pointer;
        using void_pointer = typename my_base::void_pointer;
        using const_void_pointer = typename my_base::const_void_pointer;
        using size_type = std::size_t;
        using iterator = poly_vector_iterator<interface_type ,interface_allocator_type, CloningPolicy>;
        using const_iterator = poly_vector_iterator<const interface_type,const interface_allocator_type, const CloningPolicy>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using move_is_noexcept_t = std::is_nothrow_move_assignable<my_base>;
        using cloning_policy_traits = cloning_policy_traits<CloningPolicy,interface_type,interface_allocator_type>;
        using interface_type_noexcept_movable = typename cloning_policy_traits::noexcept_movable;

        static_assert(std::is_same<IF,interface_type>::value,
                      "Interface type must be a non-cv qualified user defined type");
        static_assert(std::is_polymorphic<interface_type>::value, "interface_type is not polymorphic");
        static_assert(is_cloning_policy<CloningPolicy,interface_type,Allocator>::value,"invalid cloning policy type");
        static constexpr auto default_avg_size = 4 * sizeof(void*);
        ///////////////////////////////////////////////
        // Ctors,Dtors & assignment
        ///////////////////////////////////////////////
        poly_vector() : _free_elem{}, _begin_storage{}, _free_storage{} {}

        explicit poly_vector(const allocator_type& alloc) : allocator_base<allocator_type>(alloc),
            _free_elem{}, _begin_storage{}, _free_storage{} {};

        poly_vector(const poly_vector& other) : allocator_base<allocator_type>(other.base()),
            _free_elem{ begin_elem() }, _begin_storage{ begin_elem() + other.capacity() },
            _free_storage{ _begin_storage }
        {
            resize_to_fit(other, other.size());
            set_ptrs(other.poly_uninitialized_copy(base().get_allocator_ref(),begin_elem(), other.size()));
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
                copy_assign_impl(rhs);
            }
            return *this;
        }

        poly_vector& operator=(poly_vector&& rhs) noexcept(move_is_noexcept_t::value)
        {
            if (this != &rhs) {
                move_assign_impl(std::move(rhs), move_is_noexcept_t{});
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
            base().destroy(std::addressof(back()));
            _free_storage = static_cast<pointer>(_free_storage) - begin_elem()[size() - 1].size();
            base().destroy(std::addressof(begin_elem()[size() - 1]));
            _free_elem = &begin_elem()[size() - 1];
        }

        void clear() noexcept
        {
            for (auto src = begin_elem(); src != _free_elem; ++src) {
                base().destroy(src->ptr.second);
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
            return const_iterator(begin_elem());
        }
        const_iterator end()const noexcept
        {
            return const_iterator(end_elem());
        }
        reverse_iterator rbegin()noexcept
        {
            return reverse_iterator(--end());
        }
        reverse_iterator rend()noexcept
        {
            return reverse_iterator(--begin());
        }
        const_iterator rbegin()const noexcept
        {
            return const_iterator(--end());
        }
        const_iterator rend()const noexcept
        {
            return const_iterator(--begin());
        }
        ///////////////////////////////////////////////
        // Capacity
        ///////////////////////////////////////////////
        size_t size()const noexcept
        {
            return static_cast<size_t>(_free_elem-begin_elem());
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
            return std::make_pair(capacity(), storage_size(_begin_storage, this->_end_storage));
        }
        bool empty() const noexcept
        {
            return begin_elem() == _free_elem;
        }
        size_type max_size() const noexcept
        {
            auto avg = avg_obj_size() ? avg_obj_size() : 4 * sizeof(void_pointer);
            return allocator_traits::max_size(this->get_allocator_ref()) /
                (sizeof(elem_ptr) + avg);
        }

        void reserve(size_type n, size_type avg_size)
        {
            using copy = std::conditional_t<interface_type_noexcept_movable::value,
                    std::false_type,std::true_type>;
            if (n <= capacities().first && avg_size <= capacities().second) return;
            if (n > max_size())throw std::length_error("poly_vector reserve size too big");
            increase_storage(*this, *this, n, avg_size, alignof(std::max_align_t),copy{});
        }

        void reserve(size_type n) {
            reserve(n, default_avg_size);
        }
        void reserve(std::pair<size_t, size_t> s)
        {
            reserve(s.first, s.second);
        }
        // TODO(fecjanky): void shrink_to_fit();
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
        std::pair<void*, void*> data() noexcept{
            return std::make_pair(base()._storage,base()._end_storage);
        }
        std::pair<const void*,const void*> data() const noexcept {
            return std::make_pair(base()._storage, base()._end_storage);
        }
        ////////////////////////////
        // Misc.
        ////////////////////////////
        allocator_type get_allocator() const noexcept
        {
            return my_base::get_allocator_ref();
        }
    private:
        using elem_ptr = poly_vector_elem_ptr<typename allocator_traits::template rebind_alloc<interface_type>, CloningPolicy>;
        using elem_ptr_pointer = typename allocator_traits::template rebind_traits<elem_ptr>::pointer;
        using elem_ptr_const_pointer = typename allocator_traits::template rebind_traits<elem_ptr>::const_pointer;
        using alloc_descr_t = std::pair<bool, std::pair<size_t, size_t>>;
        using poly_copy_descr = std::tuple<elem_ptr_pointer , elem_ptr_pointer, void_pointer >;
        typedef poly_copy_descr(poly_vector::*copy_mem_fun)(elem_ptr_pointer , size_t) const;

        static alloc_descr_t make_alloc_descr(bool b, size_t size, size_t align)noexcept
        {
            return std::make_pair(b, std::make_pair(size, align));
        }

        static void_pointer next_aligned_storage(void_pointer p, size_t align) noexcept
        {
            auto v = static_cast<pointer>(p) - static_cast<pointer>(nullptr);
            auto a = ((v + align - 1)/align)*align - v;
            return static_cast<pointer>(p) + a;
        }

        static void* next_storage_for_object(void_pointer s, size_t size, size_t align) noexcept
        {
            s = next_aligned_storage(s, align);
            s = static_cast<pointer>(s) + size;
            return s;
        }

        static size_t estimate_excess(void_pointer s, void_pointer e, alloc_descr_t n, size_t avg) noexcept
        {
            auto es = storage_size(static_cast<pointer>(s) + n.second.first*avg, e);
            auto en = (es + (sizeof(elem_ptr) + avg) - 1) / (sizeof(elem_ptr) + avg);
            return std::max(size_t(1), en);
        }

        static size_t storage_size(const_void_pointer b, const_void_pointer e) noexcept
        {
            using const_pointer_t = typename allocator_traits::const_pointer;
            return static_cast<size_t>(static_cast<const_pointer_t>(e)-static_cast<const_pointer_t>(b));
        }

        template<typename CopyOrMove>
        static void increase_storage(const poly_vector& src, poly_vector& dst,
            size_t desired_size, size_t curr_elem_size, size_t align ,CopyOrMove)
        {
            alloc_descr_t n{ false,std::make_pair(desired_size,std::size_t(0)) };

            my_base s{};
            while (!n.first) {
                n.second = src.calc_increased_storage_size(n.second.first, curr_elem_size, align);
                my_base a(n.second.second, dst.get_allocator_ref());
                s.swap(a);
                n = src.validate_layout(s.storage(), s.end_storage(), n, curr_elem_size, align);
                if (!n.first) n.second.first *= 2;
            }
            dst.obtain_storage(src, dst, std::move(s), n.second.first, CopyOrMove{});
        }

        static void obtain_storage(const poly_vector& src, poly_vector& dst, my_base&& a, size_t n,
            std::true_type)
        {
            auto ret = src.poly_uninitialized_copy(dst.base().get_allocator_ref(),
                                                   static_cast<elem_ptr_pointer>(a.storage()), n);
            dst.tidy();
            dst.base().swap(a);
            dst.set_ptrs(ret);
        }

        static void obtain_storage(const poly_vector& src, poly_vector& dst, my_base&& a, size_t n,
           std::false_type) noexcept
        {
            auto ret = src.poly_uninitialized_move(dst.base().get_allocator_ref(),
                                                   static_cast<elem_ptr_pointer>(a.storage()), n);
            dst.tidy();
            dst.base().swap(a);
            dst.set_ptrs(ret);
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
            auto begin = static_cast<pointer>(nullptr);
            const auto first_align = !empty() ? begin_elem()->align() : size_t(1);
            auto end = static_cast<pointer>(
                next_aligned_storage(begin + desired_size * sizeof(elem_ptr), first_align));
            end += storage_size(_begin_storage, this->end_storage());
            end = static_cast<pointer>(next_aligned_storage(end, align));
            end += (desired_size - size())*s;
            return std::make_pair(desired_size, storage_size(begin, end));
        }

        alloc_descr_t validate_layout(void_pointer const start, void_pointer const end, const alloc_descr_t n,
            const size_t size, const size_t align) const noexcept
        {
            pointer const new_begin_storage =
                    static_cast<pointer>(start) + n.second.first * sizeof(elem_ptr);
            void_pointer new_end_storage = new_begin_storage;
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

        poly_copy_descr poly_uninitialized_copy(const allocator_type& a,elem_ptr_pointer dst, size_t n) const
        {
            auto dst_begin = dst;
            const auto storage_begin = dst + n;
            void_pointer dst_storage = storage_begin;
            try {
                for (auto elem = begin_elem(); elem != _free_elem; ++elem, ++dst) {
                    base().construct(static_cast<elem_ptr_pointer>(dst),*elem);
                    dst->ptr.first = next_aligned_storage(dst_storage, elem->align());
                    dst->ptr.second = elem->policy().clone(a,elem->ptr.second, dst->ptr.first);
                    dst_storage = static_cast<pointer>(dst->ptr.first) + dst->size();
                }
                return std::make_tuple(dst, storage_begin, dst_storage);
            } catch (...) {
                base().destroy(dst);
                while(dst-- != dst_begin)
                {
                    base().destroy(dst->ptr.second);
                    base().destroy(dst);
                }
                throw;
            }
        }

        poly_copy_descr poly_uninitialized_move(const allocator_type& a,elem_ptr_pointer dst, size_t n) const noexcept
        {
            auto dst_begin = dst;
            const auto storage_begin = dst + n;
            void_pointer dst_storage = storage_begin;
            for (auto elem = begin_elem(); elem != _free_elem; ++elem, ++dst) {
                base().construct(static_cast<elem_ptr_pointer>(dst),*elem);
                dst->ptr.first = next_aligned_storage(dst_storage, elem->align());
                dst->ptr.second =
                        elem->policy().move(a,elem->ptr.second, dst->ptr.first);
                dst_storage = static_cast<pointer>(dst->ptr.first) + dst->size();
            }
            return std::make_tuple(dst, storage_begin, dst_storage);
        }

        poly_vector& copy_assign_impl(const poly_vector& rhs)
        {
            base().get_allocator_ref() = rhs.base().get_allocator_ref();
            increase_storage(rhs, *this, rhs.size(), 0, 1, std::true_type{});
            return *this;
        }

        poly_vector& move_assign_impl(poly_vector&& rhs, std::true_type) noexcept
        {
            using std::swap;
            base().swap(rhs.base());
            swap_ptrs(std::move(rhs));
            return *this;
        }

        poly_vector& move_assign_impl(poly_vector&& rhs, std::false_type)
        {
            if (base().get_allocator_ref() == rhs.base().get_allocator_ref()) {
                return move_assign_impl(std::move(rhs), std::true_type{});
            }
            else
                return copy_assign_impl(rhs);
        }

        void tidy() noexcept
        {
            clear();
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
            using traits = typename  allocator_traits ::template rebind_traits <TT>;

            assert(can_construct_new_elem(s, a));

            auto nas = next_aligned_storage(a);
            _free_elem = base().construct (_free_elem,std::forward<T>(obj),
                nas, base().construct(static_cast<typename traits::pointer>(nas),std::forward<T>(obj)));
            _free_storage = static_cast<pointer>(_free_elem->ptr.first) + s;
            ++_free_elem;
        }

        template<class T>
        void push_back_new_elem_w_storage_increase(T&& obj)
        {
            using TT = std::decay_t<T>;
            constexpr auto s = sizeof(TT);
            constexpr auto a = alignof(TT);
            //////////////////////////////////////////
            poly_vector v(base().get_allocator_ref());
            const auto newsize = std::max(size() * 2, size_t(1));
            v.resize_to_fit(*this, newsize, s, a);
            push_back_new_elem_w_storage_increase_copy(v, interface_type_noexcept_movable{});
            v.push_back_new_elem(std::forward<T>(obj));
            this->swap(v);
        }

        void push_back_new_elem_w_storage_increase_copy(poly_vector& v, std::true_type) {
            v.set_ptrs(poly_uninitialized_move(base().get_allocator_ref(), v.begin_elem(), v.capacity()));
        }
        void push_back_new_elem_w_storage_increase_copy(poly_vector& v, std::false_type) {
            v.set_ptrs(poly_uninitialized_copy(base().get_allocator_ref(),v.begin_elem(), v.capacity()));
        }

        void resize_to_fit(const poly_vector& v,std::size_t new_size,std::size_t size = 0,std::size_t alignment = 1){
            alloc_descr_t n{ false,std::make_pair(new_size,std::size_t(0)) };
            while (!n.first) {
                n = v.validate_layout(this->storage(), this->end_storage(), n, size, alignment);
                if (!n.first) {
                    this->reserve(n.second.first*2,v.new_avg_obj_size(size,alignment));
                    n.second.first = this->capacity();
                }
            }
        }

        bool can_construct_new_elem(size_t s, size_t align) noexcept
        {
            auto free = static_cast<pointer>(next_aligned_storage(_free_storage, align));
            return free + s <= this->end_storage();
        }

        void_pointer next_aligned_storage(size_t align) const noexcept
        {
            return next_aligned_storage(_free_storage, align);
        }

        size_t new_avg_obj_size(size_t new_object_size, size_t align)const noexcept
        {
            return (storage_size(_begin_storage, static_cast<pointer>(next_aligned_storage(align)) + new_object_size)
                + this->size()) / (this->size() + 1);
        }

        size_t avg_obj_size(size_t align = 1)const noexcept
        {
            return size() ?
                   (static_cast<size_t>(storage_size(_begin_storage, next_aligned_storage(align))) + size() - 1) / size() : 0;
        }

        elem_ptr_pointer begin_elem()noexcept
        {
            return static_cast<elem_ptr_pointer>(this->storage());
        }

        elem_ptr_pointer end_elem()noexcept
        {
            return static_cast<elem_ptr_pointer>(_free_elem);
        }

        elem_ptr_const_pointer begin_elem()const noexcept
        {
            return static_cast<elem_ptr_const_pointer>(this->storage());
        }

        elem_ptr_const_pointer end_elem()const noexcept
        {
            return static_cast<elem_ptr_const_pointer>(_free_elem);
        }
        ////////////////////////////
        // Members
        ////////////////////////////
        elem_ptr_pointer    _free_elem;
        void_pointer        _begin_storage;
        void_pointer        _free_storage;
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
