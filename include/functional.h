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

template<typename... F>
struct IFunction;

template<typename R,typename... Args,typename... F>
struct IFunction<R(Args...),F...> : public IFunction<F...>{
    using IFunction<F...>::call_function__;
    virtual R call_function__(Args...) = 0;
};

template<typename R,typename... Args>
struct IFunction<R(Args...)> {
    virtual R call_function__(Args...) = 0;
    virtual ~IFunction() = default;
};


template<typename... F>
struct IInterface : public IFunction<F...>{
//    virtual IInterface* clone() = 0;
//    virtual IInterface* clone(IInterface* dest) = 0;
};

template<typename IF,typename Impl,typename... F>
struct Binder;

template<typename IF,typename Impl,typename R,typename... Args,typename... F>
struct Binder<IF,Impl,R(Args...),F...> : public Binder<IF,Impl,F...>{

    Binder(const Impl& i) : Binder<IF,Impl,F...>(i){}

    R call_function__(Args... args) override{
        return this->Impl::operator()(args...);
    }
};

template<typename IF,typename Impl,typename... Args,typename... F>
struct Binder<IF,Impl,void(Args...),F...> : public Binder<IF,Impl,F...>{

    Binder(const Impl& i) : Binder<IF,Impl,F...>(i){}

    void call_function__(Args... args) override{
       this->Impl::operator()(args...);
    }
};

template<typename IF,typename Impl,typename R,typename... Args>
struct Binder<IF,Impl,R(Args...)> : public IF, protected Impl {

    Binder(const Impl& i) : Impl(i){}

    R call_function__(Args... args) override {
        return this->Impl::operator()(args...);
    }
};

template<typename IF,typename Impl,typename... Args>
struct Binder<IF,Impl,void(Args...)> : public IF, protected Impl {

    Binder(const Impl& i) : Impl(i){}

    void call_function__(Args... args) override {
        this->Impl::operator()(args...);
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
        return i.operator()<void>(std::forward<AArgs>(args)...);
    }

private:
    IF& i;
};

// TODO(fecjanky): Add Small Object Optimization
// TODO(fecjanky): Add Copy constructor for interface (clone() function...)
template<typename... Fs>
struct interface{



    using if_t = impl::IInterface<Fs...>;

    template<
        typename T,
        typename = std::enable_if_t< !std::is_base_of<interface,T>::value >
    > interface(const T& t) : impl{new impl::Binder<if_t,T,Fs...>(t)} {
    }

    template<typename R,typename... Args>
    std::enable_if_t<
        !std::is_same<void,R>::value,
        R
    > operator()(Args... args){
        check();
        return impl->call_function__(args...);
    }

    template<typename R,typename... Args>
    std::enable_if_t<
      std::is_same<void,R>::value
    > operator()(Args... args){
        check();
        impl->call_function__(args...);
    }

    // Copying not supported yet
    interface(const interface&& i)  = delete;
    // Copying assignmentnot supported yet
    interface& operator = (const interface&& i) = delete;

    interface(interface&& i) noexcept : impl{nullptr} {
        std::swap(impl,i.impl);
    }

    interface& operator= (interface&& rhs) noexcept{
        if(this != & rhs)
            std::swap(impl,rhs.impl);
        return *this;
    }

    template<typename F>
    function_view_t<interface,F> function_view() noexcept{
        return function_view_t<interface,F>(*this);
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
