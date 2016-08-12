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
#include "memory.h"

namespace estd
{
    template<class Allocator = std::allocator<uint8_t> >
    struct allocator_base : private Allocator
    {

        using allocator_traits = std::allocator_traits<Allocator>;
        using value_type = typename allocator_traits::value_type;
        using propagate_on_container_copy_assignment = typename allocator_traits::propagate_on_container_copy_assignment;
        using propagate_on_container_swap = typename allocator_traits::propagate_on_container_swap;
        using propagate_on_container_move_assignment = typename allocator_traits::propagate_on_container_move_assignment;
        static_assert(std::is_same<value_type, uint8_t>::value, "requires a byte allocator");
        using allocator_type = Allocator;
        using void_ptr = void*;
        //////////////////////////////////////////////
        allocator_base( const Allocator& a = Allocator() ) : Allocator( a ), _storage{}, _end_storage{} {}
        allocator_base( size_t n, const Allocator& a = Allocator() ) : Allocator( a ), _storage{}, _end_storage{}
        {
            auto s = get_allocator_ref().allocate( n );
            _storage = s;
            _end_storage = s + n;
        }

        allocator_base( const allocator_base& a ) :
            allocator_base( allocator_traits::select_on_container_copy_construction( a.get_allocator_ref() ) )
        {
            if (a.size())allocate( a.size() );
        }
        allocator_base& operator=( allocator_base& a )
        {
            return copy_assign_impl( a, propagate_on_container_copy_assignment{} );
        }
        allocator_base( allocator_base&& a ) noexcept :
        Allocator( std::move( a.get_allocator_ref() ) ), 
            _storage{ a._storage }, _end_storage{ a._end_storage }
        {
            a._storage = a._end_storage = nullptr;
        }
        allocator_base& operator=( allocator_base&& a ) noexcept
        {
            // TODO: check for allocator traits
            using std::swap;
            swap( a._storage, _storage );
            swap( a._end_storage, _end_storage );
            return *this;
        }

        void swap( allocator_base& x ) noexcept
        {
            using std::swap;
            if (propagate_on_container_swap::value) {
                swap( get_allocator_ref(), x.get_allocator_ref() );
            } else if (!::estd::impl::allocator_is_always_equal_t<Allocator>::value &&
                get_allocator_ref() != x.get_allocator_ref()) {
                // Undefined behavior
                assert( 0 );
            }
            swap( _storage, x._storage );
            swap( _end_storage, x._end_storage );
        }
        allocator_base& copy_assign_impl( const allocator_base& a, std::true_type )
        {
            // Note: allocator copy assignment must not throw
            auto s = a.get_allocator_ref().allocate( n );
            tidy();
            get_allocator_ref() = a.get_allocator_ref();
            _storage = s;
            _end_storage = s + n;
            return *this;
        }
        allocator_base& copy_assign_impl( const allocator_base& a, std::false_type )
        {
            auto s = get_allocator_ref().allocate( n );
            tidy();
            _storage = s;
            _end_storage = s + n;
            return *this;
        }

        void allocate( size_t n )
        {
            auto s = get_allocator_ref().allocate( n );
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
            get_allocator_ref().deallocate( reinterpret_cast<uint8_t*>(_storage), size() );
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
        poly_vector_elem_ptr() : size{}, sf{} {}
        template<typename T, typename = std::enable_if_t< std::is_base_of< IF, std::decay_t<T> >::value > >
        explicit poly_vector_elem_ptr( T&& t, void* s = nullptr, IF* i = nullptr ) noexcept :
            CloningPolicy( std::forward<T>( t ) ), ptr{ s, i }, sf{ &size_func<std::decay_t<T>> }
        {}
        poly_vector_elem_ptr( const poly_vector_elem_ptr& other ) noexcept:
            CloningPolicy( other.policy() ), ptr{ other.ptr },sf{other.sf}
        {}
        poly_vector_elem_ptr& operator=( const poly_vector_elem_ptr & rhs ) noexcept
        {
            policy() = rhs.policy();
            ptr = rhs.ptr;
            sf = rhs.sf;
        }
        CloningPolicy& policy()noexcept
        {
            return *this;
        }
        const CloningPolicy& policy()const noexcept
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
            return std::make_pair( sizeof( T ), alignof(T) );
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
    struct DefaultCloningPolicy
    {
        DefaultCloningPolicy() = default;
        template<typename T>
        DefaultCloningPolicy( T&& t ) {};
        IF* clone( IF* obj, void* dest ) const
        {
            return obj->clone( dest );
        }
    };

    template<class Interface>
    struct delegate_cloning_policy
    {
        typedef Interface* (*clone_func_t)(const Interface* obj, void* dest);

        delegate_cloning_policy() :cf{} {}

        template < 
            typename T, 
            typename = std::enable_if_t<!std::is_same<delegate_cloning_policy,std::decay_t<T>>::value >
        >
        delegate_cloning_policy( T&& t ) noexcept :
            cf ( & delegate_cloning_policy::clone_func< std::decay_t<T> > ){};

        delegate_cloning_policy( const delegate_cloning_policy & other ) noexcept : cf { other.cf }{}
        
        template<class T>
        static Interface* clone_func( const Interface* obj, void* dest )
        {
            return new(dest) T( *static_cast<const T*>(obj) );
        }

        Interface* clone( Interface* obj, void* dest ) const
        {
            return cf(obj,dest);
        }

        /////////////////////////
        clone_func_t cf;
    };


    template<class IF,class CP>
    class poly_vector_iterator
    {
    public:
        using void_ptr = void*;
        using interface_type = IF;
        using interface_ptr = interface_type*;
        using elem_ptr = poly_vector_elem_ptr<IF, CP>;

        poly_vector_iterator() : curr{} {}
        explicit poly_vector_iterator( elem_ptr* p ) : curr{ p } {}
        poly_vector_iterator( const poly_vector_iterator& ) = default;
        poly_vector_iterator& operator=( const poly_vector_iterator& ) = default;
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
        poly_vector_iterator operator++( int )noexcept
        {
            poly_vector_iterator i( *this );
            ++curr;
            return i;
        }
        poly_vector_iterator& operator--()noexcept
        {
            --curr;
            return *this;
        }
        poly_vector_iterator operator--( int )noexcept
        {
            poly_vector_iterator i( *this );
            --curr;
            return i;
        }
        bool operator==( const poly_vector_iterator& rhs )const noexcept
        {
            return curr == rhs.curr;
        }
        bool operator!=( const poly_vector_iterator& rhs )const noexcept
        {
            return curr != rhs.curr;
        }

    private:
        elem_ptr* curr;
    };

    template<
        class IF, 
        class Allocator = std::allocator<uint8_t>, 
        class CloningPolicy = DefaultCloningPolicy<IF> 
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
        using allocator_type = Allocator;
        using allocator_traits = typename allocator_base::allocator_traits;
        using void_ptr = void*;
        using size_type = std::size_t;
        using iterator = poly_vector_iterator<interface_type,CloningPolicy>;
        using const_iterator = poly_vector_iterator<const interface_type,const CloningPolicy>;

        static constexpr auto default_avg_size = 4 * sizeof( void* );
        static_assert(std::is_polymorphic<interface_type>::value, "IF is not polymorphic");
        ///////////////////////////////////////////////
        // Ctors,Dtors & assignment
        ///////////////////////////////////////////////
        poly_vector() : _free_elem{}, _begin_storage{}, _free_storage{} {}
        ~poly_vector()
        {
            tidy();
        }
        poly_vector( const poly_vector& other ) : allocator_base<Allocator>(other.base()),
            _free_elem{begin_elem()}, _begin_storage{ begin_elem() + other.size()},
            _free_storage{_begin_storage}
        {
            auto ret = other.poly_uninitialized_copy( begin_elem(), other.size() );
            _free_elem = ret.first;
            _free_storage = ret.second;
        }
        poly_vector( poly_vector&& other ) : allocator_base<Allocator>( std::move( other.base() ) ),
            _free_elem{ other._free_elem }, _begin_storage{ other._begin_storage },
            _free_storage{ other._free_storage }
        {
            other._begin_storage = other._free_storage = other._free_elem = nullptr;
        }
        // TODO: assignment operators
        ///////////////////////////////////////////////
        // Modifiers
        ///////////////////////////////////////////////
        template<typename T>
        std::enable_if_t<std::is_base_of<interface_type, std::decay_t<T>>::value>
            push_back( T&& obj )
        {
            using TT = std::decay_t<T>;
            constexpr auto s = sizeof( TT );
            constexpr auto a = alignof(TT);
            if ( end_elem() == _begin_storage || !can_construct(s,a)) {
                increase_storage( s, a );
            }
            auto nas = next_aligned_storage( a );
            _free_elem = new (_free_elem) elem_ptr( std::forward<T>( obj ),
                nas, new (nas) TT( std::forward<T>( obj ) ) );
            _free_storage = reinterpret_cast<uint8_t*>(_free_elem->ptr.first) + s;
            ++_free_elem;
        }
        void pop_back()noexcept
        {
            back().~IF();
            _free_storage = size()>1 ? reinterpret_cast<uint8_t*>(begin_elem()[size() - 2].ptr.first) + 
                begin_elem()[size() - 2].size() : _begin_storage;
            begin_elem()[size() - 1].~poly_vector_elem_ptr();
            _free_elem = &begin_elem()[size() - 1];
        }
        void clear() noexcept
        {
            tidy();
        }
        void swap( poly_vector& x ) noexcept
        {
            using std::swap;
            base().swap( x.base() );
            swap( _free_elem, x._free_elem );
            swap( _begin_storage, x._begin_storage );
            swap( _free_storage, x._free_storage );
        }
        // TODO: insert, erase,emplace, emplace_back, assign?
        ///////////////////////////////////////////////
        // Iterators
        ///////////////////////////////////////////////
        iterator begin()noexcept
        {
            return iterator( begin_elem() );
        }
        iterator end()noexcept
        {
            return iterator( end_elem() );
        }
        const_iterator begin()const noexcept
        {
            return iterator( begin_elem() );
        }
        const_iterator end()const noexcept
        {
            return iterator( end_elem() );
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
            return std::make_pair( size(), avg_obj_size() );
        }
        size_type capacity() const noexcept
        {
            return storage_size( begin_elem(), _begin_storage ) / sizeof( elem_ptr );
        }
        std::pair<size_t, size_t> capacities() const noexcept
        {
            return std::make_pair( capacity(), storage_size( _begin_storage, _end_storage ) );
        }
        bool empty() const noexcept
        {
            return storage() == begin_elem();
        }
        size_type max_size() const noexcept
        {
            auto avg = avg_obj_size() ? avg_obj_size() : 4 * sizeof( void_ptr );
            return allocator_traits::max_size( get_allocator_ref() ) /
                (sizeof( elem_ptr ) + avg);
        }
        void reserve( size_type n ,size_type avg_size = avg_obj_size())
        {
            if (n < size()) return;
            if (n > max_size())throw std::length_error( "poly_vector reserve size too big" );
            // TODO: check if aligment criteria holds
            my_base a( storage_size_of( n, avg_size ),
                allocator_traits::select_on_container_copy_construction( get_allocator_ref() ) );
            obtain_storage( std::move( a ), size() );
        }
        // TODO: void shrink_to_fit();
        ///////////////////////////////////////////////
        // Element access
        ///////////////////////////////////////////////
        interface_reference operator[]( size_t n ) noexcept
        {
            return *begin_elem()[n].ptr.second;
        }
        const_interface_reference operator[]( size_t n )const noexcept
        {
            return *begin_elem()[n].ptr.second;
        }
        interface_reference at( size_t n )
        {
            if (n >= size()) throw std::out_of_range{ "poly_vector out of range access" };
            return (*this)[n];
        }
        const_interface_reference at( size_t n )const
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

    private:
        using my_base = allocator_base<Allocator>;
        using elem_ptr = poly_vector_elem_ptr<interface_type, CloningPolicy>;
        using alloc_descr_t = std::pair<bool, std::pair<size_t, size_t>>;

        my_base& base()noexcept
        {
            return *this;
        }
        const my_base& base()const noexcept
        {
            return *this;
        }
        size_t storage_size_of( size_t n, size_type avg_size = avg_obj_size() )const noexcept
        {
           return n*((avg_size ? avg_size : 4*sizeof(void_ptr))+sizeof(elem_ptr))
        }
        void_ptr next_aligned_storage( size_t align ) const noexcept
        {
            return next_aligned_storage( _free_storage, align );
        }
        static void_ptr next_aligned_storage(void*p, size_t align ) noexcept
        {
            auto fs = reinterpret_cast<uintptr_t>(p);
            return reinterpret_cast<void_ptr>(((fs + align - 1) / align)*align);
        }


        std::pair<elem_ptr*, void_ptr> poly_uninitialized_copy(elem_ptr* dst, size_t n ) const
        {
            auto dst_begin = dst;
            void_ptr dst_storage = dst + n;
            try {
                for (auto elem = begin_elem(); elem != _free_elem; ++elem, ++dst) {
                    dst = new (dst) elem_ptr( *elem );
                    dst->ptr.first = next_aligned_storage( dst_storage, elem->align() );
                    dst->ptr.second = elem->policy().clone( elem->ptr.second, dst->ptr.first );
                    dst_storage = reinterpret_cast<uint8_t*>(dst->ptr.first) + dst->size();
                }
                return std::make_pair( dst, dst_storage );
            } catch (...) {
                for (; dst != dst_begin; --dst) {
                    dst->ptr.second->~IF();
                }
                throw;
            }
        }
        void increase_storage( size_t curr_elem_size, size_t align )
        {
            alloc_descr_t n{false,std::make_pair(capacity(),size_type(0))};
            my_base s{};
            while (!n.first) {
                n.second = calc_increased_storage_size(n.second.first,curr_elem_size, align );
                s = my_base( n.second.second,
                    allocator_traits::select_on_container_copy_construction( get_allocator_ref() ) );
                n = validate_layout( s.storage(), s.end_storage(), n, curr_elem_size, align );
            }
            obtain_storage( std::move( s ), n.second.first );
        }
        bool can_construct( size_t s, size_t align )
        {
            auto free = reinterpret_cast<uint8_t*>(next_aligned_storage( _free_storage, align ));
            return free + s <= end_storage();
        }
        static void* next_storage_for_object( void* s, size_t size, size_t align )
        {
            s = next_aligned_storage( s, align );
            s = reinterpret_cast<uint8_t*>(s) + size;
            return s;
        }
        size_t new_avg_obj_size( size_t new_object_size )
        {
            return (this->size()*avg_obj_size() + new_object_size + this->size()) /
                (this->size() + 1);
        }
        static size_t estimate_excess( void* s, void* e, alloc_descr_t n, size_t avg )
        {
            auto es = storage_size( reinterpret_cast<uint8_t*>(s) + n.second.first*avg, e );
            auto en = (es + (sizeof( elem_ptr ) + avg) - 1) / (sizeof( elem_ptr ) + avg);
            return std::max( size_t(1), en );
        }

        alloc_descr_t validate_layout( void* const start, void* const end, const alloc_descr_t n,
            const size_t size, const size_t align )
        {
            uint8_t* const new_begin_storage =
                reinterpret_cast<uint8_t*>(start) + n.second.first*sizeof( elem_ptr );
            void* new_end_storage = new_begin_storage;
            for (auto p = begin_elem(); p != end_elem() && new_end_storage <= end; ++p) {
                new_end_storage = next_storage_for_object( new_end_storage, p->size(), p->align() );
            }
            // at least one new object should be constructed
            new_end_storage = next_storage_for_object( new_end_storage, size, align );
            //return with previous alloc descriptor
            if (new_end_storage > end)
                return n;
            //do not re-pivot begin_storage if ther is not too much headroom
            if (storage_size( new_begin_storage, end ) / new_avg_obj_size( size ) <= n.second.first) {
                return std::make_pair( true, std::make_pair( n.second.first, n.second.second ) );
            }
            // estimate excess elem no
            auto new_size = n.second.first +
                estimate_excess( new_begin_storage, end, n, new_avg_obj_size( size ) );
            // try re-pivot storage
            return validate_layout( start, end, std::make_pair( true,
                std::make_pair( new_size, n.second.second ) ), size, align );
        }

        void obtain_storage( my_base&& a, size_t n)
        {
            auto ret = poly_uninitialized_copy(
                reinterpret_cast<elem_ptr*>(a.storage()), n );
            tidy();
            my_base::operator=( std::move( a ) );
            _begin_storage = reinterpret_cast<elem_ptr*>(storage()) + n;
            _free_elem = ret.first;
            _free_storage = ret.second;
        }

        std::pair<size_type, size_type> calc_increased_storage_size(
            size_t n,size_t curr_elem_size, size_t align ) const noexcept
        {
            n = n ? n * 2 : size_t(1);
            const auto s = std::max( curr_elem_size, avg_obj_size() );
            auto begin = reinterpret_cast<uint8_t*>(nullptr);
            const auto first_align = !empty() ? begin_elem()->align() : alignof( std::max_align_t );
            auto end = reinterpret_cast<uint8_t*>(
                next_aligned_storage( begin + n*sizeof( elem_ptr ), first_align ));
            end += storage_size( _begin_storage, end_storage() );
            end = reinterpret_cast<uint8_t*>(next_aligned_storage( end, align ));
            end += n*std::max( s, align );
            return std::make_pair( n, storage_size( begin, end ) );
        }

        void tidy() noexcept
        {
            for (auto src = begin_elem(); src != _free_elem; ++src) {
                src->ptr.second->~IF();
            }
            _begin_storage = _free_storage = _free_elem = nullptr;
            my_base::tidy();
        }

        size_t avg_obj_size()const noexcept
        {
            return storage_size() ? (storage_size() + size() - 1) / size() : 0;
        }

        size_t storage_size()const noexcept
        {
            return storage_size( _begin_storage, _free_storage );
        }

        static size_t storage_size( const void* b, const void* e ) noexcept
        {
            return reinterpret_cast<const uint8_t*>(e) -
                reinterpret_cast<const uint8_t*>(b);
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
            return reinterpret_cast<const elem_ptr*>(_begin_storage);
        }
        ////////////////////////////
        // Misc.
        ////////////////////////////
        allocator_type get_allocator() const noexcept
        {
            return my_base::get_allocator_ref();
        }
        ////////////////////////////
        // Members
        ////////////////////////////
        elem_ptr* _free_elem;
        void* _begin_storage;
        void* _free_storage;
    };

    // TODO: relational operators, swap
}  // namespace estd

#endif  //POLY_VECTOR_H_
