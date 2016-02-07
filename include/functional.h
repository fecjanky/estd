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

#ifndef FUNCTIONAL_H_
#define FUNCTIONAL_H_

#include <utility>
#include <type_traits>
#include <functional>

namespace estd {
namespace impl {

template<bool,typename T1,typename T2>
struct If {
    using type = T1;
};

template<typename T1, typename T2>
struct If<false,T1,T2> {
    using type = T2;
};

template<bool b, typename T1, typename T2>
using If_t = typename If<b, T1, T2>::type;

template<bool b,bool... B>
struct And {
    static constexpr bool value = b && And<B...>::value;
};

template<bool b, bool B>
struct And<b,B> {
    static constexpr bool value = b && B;
};

template<typename... F>
struct IFunction;

template<typename R,typename... Args,typename... F>
struct IFunction<R(Args...),F...> : public IFunction<F...>{
    using IFunction<F...>::call_function__;
    virtual R call_function__(Args...) = 0;
    
    using MyT = IFunction<R(Args...), F...>;
    MyT() = default;
    MyT(const MyT&) = delete;
    MyT& operator=(const MyT&) = delete;
    MyT(MyT&&) = delete;
    MyT& operator=(MyT&&) = delete;

};

template<typename R,typename... Args>
struct IFunction<R(Args...)> {
    virtual R call_function__(Args...) = 0;
    virtual ~IFunction() = default;

    using MyT = IFunction<R(Args...)>;
    MyT() = default;
    MyT(const MyT&) = delete;
    MyT& operator=(const MyT&) = delete;
    MyT(MyT&&) = delete;
    MyT& operator=(MyT&&) = delete;
};


template<typename... F>
struct IInterface : public IFunction<F...>{
    virtual IInterface* clone_implementation__() const = 0;
};

template<typename IF,typename Impl,typename... F>
struct Binder;

template<typename IF,typename Impl,typename R,typename... Args,typename... F>
struct Binder<IF,Impl,R(Args...),F...> : public Binder<IF,Impl,F...>{

    using MyT = Binder<IF, Impl, R(Args...), F...>;
    using MyBase = Binder<IF, Impl, F...>;

    MyT(const Impl& i) : MyBase{ i } {}
    MyT(const MyT& b) : MyBase{ b } {}

    R call_function__(Args... args) override{
        return this->Impl::operator()(std::forward<Args>(args)...);
    }

};

template<typename IF,typename Impl,typename... Args,typename... F>
struct Binder<IF,Impl,void(Args...),F...> : public Binder<IF,Impl,F...>{

    using MyT = Binder<IF, Impl, void(Args...), F...>;
    using MyBase = Binder<IF, Impl, F...>;

    MyT(const Impl& i) : MyBase{ i } {}
    MyT(const MyT& b) : MyBase{ b } {}

    void call_function__(Args... args) override{
       this->Impl::operator()(std::forward<Args>(args)...);
    }

};

template<typename IF,typename Impl,typename R,typename... Args>
struct Binder<IF,Impl,R(Args...)> : public IF, public Impl {

    using MyT = Binder<IF, Impl, R(Args...)>;
    using MyBase = Impl;

    MyT(const Impl& i) : MyBase{ i } {}
    MyT(const MyT& b) : MyBase{ b } {}

    R call_function__(Args... args) override {
        return this->Impl::operator()(std::forward<Args>(args)...);
    }

};

template<typename IF,typename Impl,typename... Args>
struct Binder<IF,Impl,void(Args...)> : public IF, public Impl {

    using MyT = Binder<IF, Impl, void(Args...)>;
    using MyBase = Impl;

    MyT(const Impl& i) : MyBase{ i } {}
    MyT(const MyT& b) : MyBase{ b } {}

    void call_function__(Args... args) override {
        this->Impl::operator()(std::forward<Args>(args)...);
    }
};


template<typename IF, typename Impl, typename... F>
struct BindImpl : public Binder<IF, Impl, F...> {
    BindImpl(const Impl&i) : Binder<IF, Impl, F...>(i) {}
    BindImpl(const BindImpl& b) : Binder<IF, Impl, F...>(b) {}

    BindImpl* clone_implementation__() const override
    {
        return new BindImpl(*this);
    }
};
} // namespace impl

template<typename IF,typename T>
struct function_view_t;

template<typename IF,typename R, typename... Args>
struct function_view_t<IF,R(Args...)>{
    function_view_t(IF& ii) noexcept : i(ii){}

    template<typename... AArgs>
    R operator()(AArgs&&... args){
        static_assert(
            sizeof...(Args) == sizeof...(AArgs) &&
            impl::And<std::is_convertible<AArgs, Args>::value...>::value,
            "Interface is not callable");
        return i.operator()<R>(std::forward<AArgs>(args)...);
    }

private:
    IF& i;
};

template<typename IF,typename... Args>
struct function_view_t<IF,void(Args...)>{
    function_view_t(IF& ii) noexcept : i(ii){}

    template<typename... AArgs>
    void operator()(AArgs&&... args){
        static_assert(
            sizeof...(Args) == sizeof...(AArgs) &&
            impl::And<std::is_convertible<AArgs, Args>::value...>::value,
            "Interface is not callable");
        i.operator()<void>(std::forward<AArgs>(args)...);
    }

private:
    IF& i;
};

template<typename F, typename IF>
function_view_t<IF, F> function_view(IF& i) {
    return function_view_t<IF, F>(i);
}

// TODO(fecjanky): Add Small Object Optimization
template<typename... Fs>
struct interface{

    using if_t = impl::IInterface<Fs...>;

    template<
        typename T,
        typename = std::enable_if_t< !std::is_base_of<interface,T>::value >
    > explicit interface(const T& t) : impl{new impl::BindImpl<if_t,T,Fs...>(t)} {
    }

    template<typename R,typename... Args>
    std::enable_if_t<
        !std::is_same<void,R>::value,
        R
    > operator()(Args&&... args){
        check();
        return impl->call_function__(std::forward<Args>(args)...);
    }

    template<typename R,typename... Args>
    std::enable_if_t<
      std::is_same<void,R>::value
    > operator()(Args&&... args){
        check();
        impl->call_function__(std::forward<Args>(args)...);
    }

    interface(const interface& i) 
        : impl { i.impl ? i.impl->clone_implementation__() : nullptr }{}

    interface& operator = (const interface& i) {
        if (this != &rhs) {
            delete impl;
            impl = i.impl ? i.impl->clone_implementation__() : nullptr;
        }
        return *this;
    }

    interface(interface&& i) noexcept : impl{nullptr} {
        std::swap(impl,i.impl);
    }

    interface& operator= (interface&& rhs) noexcept{
        if(this != & rhs)
            std::swap(impl,rhs.impl);
        return *this;
    }

    ~interface(){
        delete impl;
    }

private:
    void check(){
        if (! impl ) throw std::runtime_error(
                "Invocation on interface without implementation");
    }
    if_t* impl;
};

template<typename F>
using function = interface<F>;

} // namespace estd

#endif /* FUNCTIONAL_H_ */
