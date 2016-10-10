#include <iostream>
#include <random>
#include <sstream>
#include <chrono>
#include <cstdio>
#include <unordered_set>
#include <string>
#include <poly_vector.h>
#include <iterator>

#include "bench_classes.h"

using namespace estd;

template<typename T, typename C>
T fill_poly_vector(unsigned int num, InterfaceCreator<T, C>)
{
    auto creator = InterfaceCreator<T, C>::getCreatorVector();
    std::random_device rd;
    std::uniform_int_distribution<size_t> ud(0, creator.size() - 1);
    T v;
    for (auto i = 0U; i != num; ++i) {
        creator[ud(rd)](v);
    }
    return v;
}

template<class Functor, class T>
std::chrono::milliseconds benchmark_access(const T& v, unsigned int mult_factor) {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto i = 0U; i != mult_factor; ++i) {
        for (auto it = v.begin(); it != v.end(); ++it) {
            Functor::invoke(it);
        }
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
}

struct PolyVectorFunc {
    static void invoke(estd::poly_vector<Interface>::const_iterator i) {
        i->doYourThing();
    }
};

struct UniqVectorFunc {
    static void invoke(std::vector<std::unique_ptr<Interface>>::const_iterator i) {
        (*i)->doYourThing();
    }
};

const char * const help = "usage %s <num of elems> <num of iterations> <poly_vec|unique_ptr_vec>\n";

int main(int argc, char* argv[])
try {
    unsigned int num_of_elems{}, mult_factor{};
    if (argc < 4)throw std::runtime_error("invalid num of arguments");
    {
        std::istringstream iss(argv[1]);
        iss.exceptions(std::istream::failbit | std::istream::badbit);
        iss >> num_of_elems;
    }
    {
        std::istringstream iss(argv[2]);
        iss.exceptions(std::istream::failbit | std::istream::badbit);
        iss >> mult_factor;
    }

    std::string type(argv[3]);
    if (type == "poly_vec") {
        auto pv = fill_poly_vector(num_of_elems, InterfaceCreator<estd::poly_vector<Interface>, PolyVectorCreator>());
        auto pv_duration = benchmark_access<PolyVectorFunc>(pv, mult_factor);
        std::unordered_set<std::string> vals;
        std::transform(pv.begin(),pv.end(),std::inserter(vals,vals.end()),[](const Interface& i){return i.toString();});
        std::cout << pv_duration.count() << " ms , unique_elems:" << vals.size()<<"\n";
    }
    else if (type == "unique_ptr_vec") {
        auto sv = fill_poly_vector(num_of_elems, InterfaceCreator < std::vector < std::unique_ptr<Interface>>, UniquePtrCreator>());
        auto sv_duration = benchmark_access<UniqVectorFunc>(sv, mult_factor);
        std::unordered_set<std::string> vals;
        std::transform(sv.begin(),sv.end(),std::inserter(vals,vals.end()),[](const std::unique_ptr<Interface>& i){return i->toString();});
        std::cout << sv_duration.count() << " ms, unique_elems:" << vals.size()<<"\n";
    }
    else throw std::runtime_error("invlid vector type");

    return 0;
}
catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    std::printf(help, argv[0]);
    return 1;
}
catch (...)
{
    std::cerr << "Unknown exception\n";
    return 1;
}