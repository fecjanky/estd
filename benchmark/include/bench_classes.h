#ifndef BENCH_CLASSES_H_
#define BENCH_CLASSES_H_

#include <memory>
#include <random>
#include <string>
#include <cmath>
#include <functional>
#include <vector>

struct Interface {

    virtual void doYourThing() = 0;

    virtual std::string toString() const = 0;

    virtual ~Interface() = default;
};


class Implementation1 : public Interface {
public:
    Implementation1(int seed) : _seed{ seed }, _current{} ,_pad{}{}

    void doYourThing() override
    {
        _current += _seed;
    }

    std::string toString() const override
    {
        return std::string("Implementation1(") + std::to_string(_current) + ")";
    }
private:
    int _seed;
    int _current;
    int _pad[14];
};

class Implementation2 : public Interface {
public:

    Implementation2(double op1, double op2) : _op1{ op1 }, _op2{ op2 } ,_pad{}{}

    void doYourThing() override
    {
        _op1 = pow(_op1, _op2);
    }

    std::string toString() const override
    {
        return std::string("Implementation2(") + std::to_string(_op1) + "," + std::to_string(_op2) + ")";
    }
private:
    double _op1;
    double _op2;
    int _pad[12];
};


std::vector<std::unique_ptr<char[]>>  random_allocation(){
    // simulating heap fragmentation
    std::random_device rd;
    std::uniform_int_distribution<int>dist_size(2048, 4096);
    std::uniform_int_distribution<int>dist_num(256, 512);
    std::vector<std::unique_ptr<char[]>> tempvec;
    auto num = dist_num(rd);
    for(auto i = 0;i <num;++i){
        tempvec.push_back(std::make_unique<char[]>(dist_size(rd)));
    }
    return tempvec;
}

struct PolyVectorCreator {
    template<class T>
    static const T& create(const T& o)noexcept {
        auto v = random_allocation();
        if(v.size()>0)
            return o;
        else throw std::runtime_error("random alloc failed");


    }
};

struct UniquePtrCreator {
    template<class T>
    static std::unique_ptr<Interface> create(const T& o)noexcept {
        std::unique_ptr<T> p;
        {
            auto v = random_allocation();
            if (!v.empty()) {
                p = std::make_unique<T>(o);
            }
        }
        return std::unique_ptr<Interface>(p.release());
    }
};

template<class VectorT,class CreateT>
struct InterfaceCreator {

    class Implemenation1Creator
    {
    public:
        void operator()(VectorT& v) {
            std::random_device rd;
            std::uniform_int_distribution<int>dist(-100, 100);
            v.push_back(CreateT::create(Implementation1(dist(rd))));
        }
    private:
    };

    class Implemenation2Creator
    {
    public:
        void operator()(VectorT& v) {
            std::random_device rd;
            std::uniform_real_distribution<double> dist(1.0, 2.0);
            auto param1 = dist(rd);
            auto param2 = dist(rd);
            v.push_back(CreateT::create(Implementation2(param1, param2)));
        }
    private:
    };

    using creator_func_t = std::function<void(VectorT&)>;

    static std::vector<creator_func_t> getCreatorVector() {
        return{ Implemenation1Creator(),Implemenation2Creator() };
    }
};


#endif  //BENCH_CLASSES_H_



