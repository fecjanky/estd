#pragma once
#include "poly_vector.h"
#include "gtest/gtest.h"
#include <atomic>
#include <memory>
#include <cassert>
#include <stdexcept>
#include <iostream>

class Cookie {
public:

    static constexpr uint32_t constructed_cookie = 0xdeadbeef;
    Cookie() :cookie{} {}
    Cookie(const Cookie& other) noexcept {
        if (other.cookie != constructed_cookie) {
            std::cerr << "cookie value was " << other.cookie << std::endl;
        }
        assert(other.cookie == constructed_cookie);
    }
    Cookie(const Cookie&& other) noexcept {
        if (other.cookie != constructed_cookie) {
            std::cerr << "cookie value was " << other.cookie << std::endl;
        }
        assert(other.cookie == constructed_cookie);
    }

    Cookie& operator=(const Cookie& other) noexcept {
        if (other.cookie != constructed_cookie) {
            std::cerr << "cookie value was " << other.cookie << std::endl;
        }
        assert(other.cookie == constructed_cookie);
        return *this;
    }
    Cookie& operator=(const Cookie&& other) noexcept {
        if (other.cookie != constructed_cookie) {
            std::cerr << "cookie value was " << other.cookie << std::endl;
        }
        assert(other.cookie == constructed_cookie);
        return *this;
    }

    ~Cookie() {
        if (cookie != constructed_cookie) {
            std::cerr << "cookie value was " << cookie << std::endl;
        }
        assert(cookie == constructed_cookie);
        cookie = 0;
    }
    void set()noexcept
    {
        cookie = 0xdeadbeef;
    }
private:
    uint32_t cookie;
};
struct Interface
{
    explicit Interface(bool throw_on_copy = false) : my_id{ last_id++ }, throw_on_copy_construction{ throw_on_copy } {}
    Interface(const Interface& i) : my_id{ i.my_id }, throw_on_copy_construction{i.throw_on_copy_construction} {
        if (i.throw_on_copy_construction)
            throw std::runtime_error("Interface copy attempt with throw on copy set");
    }
    Interface(Interface&& i) noexcept : my_id{ i.my_id }, throw_on_copy_construction{ i.throw_on_copy_construction } {
        i.my_id = last_id++;
    }
    Interface& operator=(const Interface& i) = delete;

    virtual void function() = 0;
    virtual Interface* clone(void*) = 0;
    virtual Interface* move(void* dest) = 0;
    virtual ~Interface() = default;
    bool operator<(const Interface& rhs)const noexcept
    {
        return my_id < rhs.my_id;
    }
    bool operator==(const Interface& rhs)const noexcept
    {
        return my_id == rhs.my_id;
    }
    bool operator!=(const Interface& rhs)const noexcept
    {
        return my_id != rhs.my_id;
    }
    void set_throw_on_copy_construction(bool val) noexcept {
        throw_on_copy_construction = val;
    }
private:
    size_t my_id;
    static std::atomic<size_t> last_id;
protected:
    bool throw_on_copy_construction;

};

class Impl1 : public Interface
{
public:
    explicit Impl1(double dd = 0.0, bool throw_on_copy = false) : Interface{ throw_on_copy }, d{ dd }, p{ new int[111] } {
        cookie.set();
    }
    Impl1(const Impl1& i) : Interface(i), d{ i.d }, p{ new int[111] } {
        cookie.set();
    }
    Impl1(Impl1&& o)noexcept : Interface(std::move(o)), d{ o.d }, p{ new int[111] } {
        cookie.set();
    }
    void function() override
    {
        d += 1.0;
    }
    virtual Interface* clone(void* dest) override
    {
        return new (dest) Impl1(*this);
    }
    virtual Interface* move(void* dest)  override
    {
        return new (dest) Impl1(std::move(*this));
    }
private:
    double d;
    std::unique_ptr<int[]> p;
    Cookie cookie;
};

template<typename cloningp>
class alignas(alignof(std::max_align_t) * 4) Impl2T : public Interface
{
public:
    explicit Impl2T(bool throw_on_copy = false) : Interface{ throw_on_copy }, p { new double[137] } {
        cookie.set();
    }
    Impl2T(const Impl2T& o) : Interface(o), p{ new double[137] } {
        cookie.set();
    }

    Impl2T(Impl2T&& o)noexcept : v{ std::move(o.v) }, p{ std::move(o.p) } {
        cookie.set();
    }
    void function() override
    {
        v.push_back(Impl1{ 3.1 });
    }
    virtual Interface* clone(void* dest) override
    {
        return new (dest) Impl2T(*this);
    }
    virtual Interface* move(void* dest) override
    {
        return new (dest) Impl2T(std::move(*this));
    }

private:
    estd::poly_vector<Interface, std::allocator<Interface>, cloningp> v;
    std::unique_ptr<double[]> p;
    Cookie cookie;
};

using Impl2 = Impl2T<estd::virtual_cloning_policy<Interface>>;
