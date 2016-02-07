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

#include "functional.h"

#include <vector>
#include <iostream>
#include <string>

#include "memory.h"


///////////////////////////////////////////////////////
// Example
///////////////////////////////////////////////////////

struct get_name_t{};
struct set_name_t{};

struct MyImpl {
    double operator()(int,double) {
        return 3.14;
    }
    int operator()(std::vector<int>& v) {
        v.push_back(2);
        return 1;
    }

    void operator()(std::vector<int>& v,int){
        v.push_back(8);
    }

    std::string operator()(get_name_t){
        return "MyImpl";
    }

    void operator()(set_name_t,const std::string& s){
        std::cout << s << std::endl;
    }

};

struct MyImpl2{
    double operator()(int,double) {
        return 6.28;
    }
    int operator()(std::vector<int>& v) {
        v.push_back(4);
        return 2;
    }

    void operator()(std::vector<int>& v,int){
        v.push_back(16);
    }

    std::string operator()(get_name_t){
        return "MyImpl2";
    }

    void operator()(set_name_t,const std::string& s){
        std::cout << s << std::endl;
        std::cout << s << std::endl;
    }
};


template<typename T>
using reference_t = std::reference_wrapper<T>;

using func_1 = double(int,double);
using func_2 = int(reference_t<std::vector<int>>);
using func_3 = void(reference_t<std::vector<int>>,int);


using func_4 = std::string(get_name_t);
using func_5 = void(set_name_t,reference_t<const std::string>);

int main() {
    using namespace estd;
    std::vector<int> v{};

    estd::interface<func_1,func_2,func_3,func_4,func_5> i (MyImpl{});
    estd::interface<func_1,func_2,func_3,func_4,func_5> i2 (MyImpl2{});

    static_assert(std::is_same<decltype(i),decltype(i2)>::value ,"Oh shit...");

    double d = i.function_view<func_1>()(3,3.14);
    int ret = i.function_view<func_2>()(std::ref(v));

    std::cout << d << "   " << ret << std::endl;
    std::cout << i2.function_view<func_1>()(3,3.14) << "   "
            << i2.function_view<func_2>()(std::ref(v)) << std::endl;

    std::cout << v.size() << std::endl;

    i2.function_view<func_3>()(std::ref(v),0);

    std::string m = "MyName";
    i.function_view<func_5>()(set_name_t{}, std::cref(m));
    i2.function_view<func_5>()(set_name_t{}, std::cref(m));
    std::cout << i.function_view<func_4>()(get_name_t{}) << std::endl;
    std::cout << i2.function_view<func_4>()(get_name_t{}) << std::endl;
    std::cout << v.size() << std::endl;

    obj_storage o;
    o.allocate(4);
	return 0;
}
