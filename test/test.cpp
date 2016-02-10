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

    void operator()(set_name_t,std::string&& s){
        std::cout << s << std::endl;
        auto ss = std::move(s);
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


using func_1 = double(int,double);
using func_2 = int(std::vector<int>&);
using func_3 = void(std::vector<int>&,int);
using func_4 = std::string(get_name_t);
using func_5 = void(set_name_t,std::string&&);

int main() {
    using namespace estd;
    std::vector<int> v{};
    using my_if = estd::interface<func_1, func_2, func_3, func_4, func_5>;
    my_if i (MyImpl{});
    std::cout << sizeof(my_if) << std::endl;
    my_if i2 = i;
    my_if i3 = std::move(i);
    i = std::move(i3);
    //estd::interface<func_1,func_2,func_3,func_4,func_5> i2 (MyImpl2{});
    //static_assert(std::is_same<decltype(i),decltype(i2)>::value ,"Oh shit...");
    std::string s = "hello";
    //std::reference_wrapper<std::string&&> sr(std::move(s));
    function_view<func_1>(i)(3, 3.4);
    function_view<func_3>(i)(v,0);
    function_view<func_5>(i)(set_name_t{}, std::move(s));
    obj_storage o;
    o.allocate(4);
    return 0;
}
